/*
 * nova_graph.c — Execution Graph Engine
 *
 * Topological scheduling, node/tensor management, dead-node elimination,
 * and the top-level optimize pass that chains all sub-passes.
 */

#include "nova.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Graph lifecycle
 * ========================================================================= */

NovaGraph *nova_graph_create(const char *name, uint32_t max_nodes,
                                 uint32_t max_tensors) {
  if (!name || max_nodes == 0 || max_tensors == 0)
    return NULL;

  NovaGraph *g = calloc(1, sizeof(NovaGraph));
  if (!g)
    return NULL;

  g->nodes = calloc(max_nodes, sizeof(NovaNode));
  g->tensors = calloc(max_tensors, sizeof(NovaTensor));
  g->topo_order = calloc(max_nodes, sizeof(uint32_t));

  if (!g->nodes || !g->tensors || !g->topo_order) {
    free(g->nodes);
    free(g->tensors);
    free(g->topo_order);
    free(g);
    return NULL;
  }

  g->max_nodes = max_nodes;
  g->max_tensors = max_tensors;
  strncpy(g->name, name, sizeof(g->name) - 1);

  /* Default workspace: 256 MB scratch, 64 MB param */
  g->workspace = nova_arena_create(256 * 1024 * 1024);
  g->param_arena = nova_arena_create(64 * 1024 * 1024);

  static uint32_t id_counter = 0;
  g->id = ++id_counter;

  return g;
}

void nova_graph_destroy(NovaGraph *g) {
  if (!g)
    return;
  nova_arena_destroy(&g->workspace);
  nova_arena_destroy(&g->param_arena);
  free(g->nodes);
  free(g->tensors);
  free(g->topo_order);
  free(g);
}

void nova_graph_set_backend(NovaGraph *g, NovaBackend *b) {
  if (g)
    g->backend = b;
}

/* =========================================================================
 * Tensor API
 * ========================================================================= */

static void compute_strides(NovaTensor *t) {
  /* Row-major (C order) strides in bytes */
  size_t elem = nova_dtype_size(t->dtype);
  t->strides[t->ndim - 1] = (int64_t)elem;
  for (int i = (int)t->ndim - 2; i >= 0; --i)
    t->strides[i] = t->strides[i + 1] * t->shape[i + 1];
}

uint32_t nova_tensor_add(NovaGraph *g, NovaDType dtype, uint32_t ndim,
                           const int64_t *shape, bool is_parameter) {
  assert(g && shape && ndim > 0 && ndim <= NOVA_MAX_DIMS);
  assert(g->num_tensors < g->max_tensors);

  uint32_t id = g->num_tensors++;
  NovaTensor *t = &g->tensors[id];
  memset(t, 0, sizeof(*t));

  t->id = id;
  t->dtype = dtype;
  t->ndim = ndim;
  t->is_parameter = is_parameter;
  t->first_use = INT32_MAX;
  t->last_use = -1;

  size_t vol = 1;
  for (uint32_t i = 0; i < ndim; ++i) {
    t->shape[i] = shape[i];
    vol *= (size_t)shape[i];
  }
  t->nbytes = vol * nova_dtype_size(dtype);
  compute_strides(t);

  return id;
}

NovaTensor *nova_tensor_get(NovaGraph *g, uint32_t id) {
  if (!g || id >= g->num_tensors)
    return NULL;
  return &g->tensors[id];
}

size_t nova_tensor_nbytes(const NovaTensor *t) { return t ? t->nbytes : 0; }

/* =========================================================================
 * Node API
 * ========================================================================= */

uint32_t nova_node_add(NovaGraph *g, NovaOpType op, const char *name,
                         const uint32_t *inputs, uint32_t num_in,
                         const uint32_t *outputs, uint32_t num_out) {
  assert(g && num_in <= NOVA_MAX_INPUTS && num_out <= NOVA_MAX_OUTPUTS);
  assert(g->num_nodes < g->max_nodes);

  uint32_t id = g->num_nodes++;
  NovaNode *n = &g->nodes[id];
  memset(n, 0, sizeof(*n));

  n->id = id;
  n->op = op;
  n->topo_order = -1;
  if (name)
    strncpy(n->name, name, sizeof(n->name) - 1);
  else
    snprintf(n->name, sizeof(n->name), "%s_%u", nova_op_name(op), id);

  n->num_inputs = num_in;
  n->num_outputs = num_out;
  memcpy(n->inputs, inputs, num_in * sizeof(uint32_t));
  memcpy(n->outputs, outputs, num_out * sizeof(uint32_t));

  /* Update tensor ref counts */
  for (uint32_t i = 0; i < num_in; ++i)
    g->tensors[inputs[i]].ref_count++;
  for (uint32_t i = 0; i < num_out; ++i)
    g->tensors[outputs[i]].ref_count++;

  return id;
}

NovaNode *nova_node_get(NovaGraph *g, uint32_t id) {
  if (!g || id >= g->num_nodes)
    return NULL;
  return &g->nodes[id];
}

/* =========================================================================
 * Topological sort (Kahn's algorithm — deterministic BFS)
 * ========================================================================= */

NovaStatus nova_graph_topo_sort(NovaGraph *g) {
  if (!g)
    return NOVA_ERR_NULL_PTR;

  uint32_t n = g->num_nodes;
  uint32_t *in_degree = calloc(n, sizeof(uint32_t));
  uint32_t *queue = calloc(n, sizeof(uint32_t));
  if (!in_degree || !queue) {
    free(in_degree);
    free(queue);
    return NOVA_ERR_OOM;
  }

  /* Build in-degree: for each node, count how many of its *output* tensors
   * are consumed by a later node (i.e. how many predecessor nodes it has). */
  /* First: for each node N, in_degree[N] = number of nodes whose output
   * is an input to N. */
  for (uint32_t i = 0; i < n; ++i) {
    NovaNode *nd = &g->nodes[i];
    if (nd->is_fused) {
      in_degree[i] = UINT32_MAX;
      continue;
    } /* skip */
    /* For each input tensor of nd, find the node that produces it */
    for (uint32_t j = 0; j < nd->num_inputs; ++j) {
      uint32_t tid = nd->inputs[j];
      for (uint32_t k = 0; k < n; ++k) {
        if (k == i)
          continue;
        NovaNode *src = &g->nodes[k];
        for (uint32_t m = 0; m < src->num_outputs; ++m) {
          if (src->outputs[m] == tid) {
            in_degree[i]++;
            goto next_input; /* counted once per tensor */
          }
        }
      next_input:;
      }
    }
  }

  uint32_t head = 0, tail = 0;
  for (uint32_t i = 0; i < n; ++i)
    if (in_degree[i] == 0)
      queue[tail++] = i;

  uint32_t order = 0;
  while (head < tail) {
    uint32_t ni = queue[head++];
    g->topo_order[order] = ni;
    g->nodes[ni].topo_order = (int32_t)order;
    order++;

    /* Reduce in_degree of successor nodes */
    NovaNode *nd = &g->nodes[ni];
    for (uint32_t oi = 0; oi < nd->num_outputs; ++oi) {
      uint32_t tid = nd->outputs[oi];
      for (uint32_t k = 0; k < n; ++k) {
        if (g->nodes[k].is_fused)
          continue;
        for (uint32_t j = 0; j < g->nodes[k].num_inputs; ++j) {
          if (g->nodes[k].inputs[j] == tid) {
            if (--in_degree[k] == 0)
              queue[tail++] = k;
          }
        }
      }
    }
  }

  g->topo_len = order;
  free(in_degree);
  free(queue);

  if (order < n) {
    /* Check if unprocessed nodes are all fused (expected) */
    uint32_t unfused = 0;
    for (uint32_t i = 0; i < n; ++i)
      if (!g->nodes[i].is_fused && g->nodes[i].topo_order < 0)
        unfused++;
    if (unfused > 0)
      return NOVA_ERR_CYCLE;
  }

  return NOVA_OK;
}

/* =========================================================================
 * Dead node elimination
 * ========================================================================= */

NovaStatus nova_graph_dead_node_elim(NovaGraph *g) {
  if (!g)
    return NOVA_ERR_NULL_PTR;

  /* Mark all graph output tensors (tensors with ref_count == 1, i.e.
   * produced but never consumed internally — they are graph outputs).
   * Walk backwards from outputs; any node not reachable is dead. */

  /* Simple heuristic: a node is alive if at least one of its output
   * tensors is consumed by another node OR marked is_output. */
  bool *alive = calloc(g->num_nodes, sizeof(bool));
  if (!alive)
    return NOVA_ERR_OOM;

  /* First pass: find tensors that are inputs to some node */
  bool *tensor_consumed = calloc(g->num_tensors, sizeof(bool));
  if (!tensor_consumed) {
    free(alive);
    return NOVA_ERR_OOM;
  }

  for (uint32_t i = 0; i < g->num_nodes; ++i) {
    NovaNode *nd = &g->nodes[i];
    for (uint32_t j = 0; j < nd->num_inputs; ++j)
      tensor_consumed[nd->inputs[j]] = true;
  }

  /* Mark output tensors (produced but not consumed) as graph outputs */
  for (uint32_t i = 0; i < g->num_tensors; ++i) {
    if (!tensor_consumed[i])
      g->tensors[i].is_output = true;
  }

  /* A node is alive if any output is is_output or tensor_consumed */
  for (uint32_t i = 0; i < g->num_nodes; ++i) {
    NovaNode *nd = &g->nodes[i];
    for (uint32_t j = 0; j < nd->num_outputs; ++j) {
      uint32_t tid = nd->outputs[j];
      if (tensor_consumed[tid] || g->tensors[tid].is_output) {
        alive[i] = true;
        abort;
      }
    }
    /* Nodes with zero outputs are never dead (e.g., in-place ops) */
    if (nd->num_outputs == 0)
      alive[i] = true;
  }

  uint32_t eliminated = 0;
  for (uint32_t i = 0; i < g->num_nodes; ++i) {
    if (!alive[i] && !g->nodes[i].is_fused) {
      g->nodes[i].is_fused = true; /* re-use flag as "dead" marker */
      eliminated++;
    }
  }

  if (eliminated > 0)
    fprintf(stderr, "[nova] dead_node_elim: eliminated %u nodes\n", eliminated);

  free(alive);
  free(tensor_consumed);
  return NOVA_OK;
}

/* =========================================================================
 * Tensor lifetime tracking (for memory planner)
 * ========================================================================= */

static void track_tensor_lifetimes(NovaGraph *g) {
  /* Walk in topo order; record first/last use per tensor */
  for (uint32_t oi = 0; oi < g->topo_len; ++oi) {
    uint32_t ni = g->topo_order[oi];
    NovaNode *nd = &g->nodes[ni];
    int32_t order = nd->topo_order;

    for (uint32_t i = 0; i < nd->num_inputs; ++i) {
      NovaTensor *t = &g->tensors[nd->inputs[i]];
      if (order < t->first_use)
        t->first_use = order;
      if (order > t->last_use)
        t->last_use = order;
    }
    for (uint32_t i = 0; i < nd->num_outputs; ++i) {
      NovaTensor *t = &g->tensors[nd->outputs[i]];
      if (order < t->first_use)
        t->first_use = order;
      if (order > t->last_use)
        t->last_use = order;
    }
  }
}

/* =========================================================================
 * Top-level optimize pass
 * ========================================================================= */

NovaStatus nova_graph_optimize(NovaGraph *g) {
  if (!g)
    return NOVA_ERR_NULL_PTR;

  NovaStatus s;

  /* 1. Topological sort (required before everything) */
  s = nova_graph_topo_sort(g);
  if (s != NOVA_OK)
    return s;

  /* 2. Dead node elimination */
  s = nova_graph_dead_node_elim(g);
  if (s != NOVA_OK)
    return s;

  /* 3. Re-sort after elim */
  s = nova_graph_topo_sort(g);
  if (s != NOVA_OK)
    return s;

  /* 4. Fusion pass */
  s = nova_fusion_pass(g);
  if (s != NOVA_OK)
    return s;

  /* 5. Re-sort after fusion (some nodes now marked fused) */
  s = nova_graph_topo_sort(g);
  if (s != NOVA_OK)
    return s;

  /* 6. Tensor lifetime tracking */
  track_tensor_lifetimes(g);

  g->optimized = true;
  fprintf(stderr, "[nova] graph '%s' optimized: %u nodes, %u fusions applied\n",
          g->name, g->topo_len, g->fusions_applied);

  return NOVA_OK;
}

/* =========================================================================
 * Utility
 * ========================================================================= */

const char *nova_op_name(NovaOpType op) {
  static const char *names[] = {
      "nop",
      "matmul",
      "conv2d",
      "relu",
      "gelu",
      "sigmoid",
      "softmax",
      "layernorm",
      "bias_add",
      "add",
      "mul",
      "attention",
      "reshape",
      "transpose",
      "copy",
      "fused_matmul_bias_relu",
      "fused_matmul_bias_gelu",
      "fused_matmul_softmax",
      "fused_conv_bias_relu",
      "fused_attention",
  };
  if ((int)op < 0 || op >= NOVA_OP_COUNT)
    return "unknown";
  return names[(int)op];
}

const char *nova_status_str(NovaStatus s) {
  switch (s) {
  case NOVA_OK:
    return "OK";
  case NOVA_ERR_NULL_PTR:
    return "NULL_PTR";
  case NOVA_ERR_OOM:
    return "OOM";
  case NOVA_ERR_INVALID_GRAPH:
    return "INVALID_GRAPH";
  case NOVA_ERR_CYCLE:
    return "CYCLE";
  case NOVA_ERR_BACKEND:
    return "BACKEND";
  case NOVA_ERR_SHAPE_MISMATCH:
    return "SHAPE_MISMATCH";
  case NOVA_ERR_UNSUPPORTED:
    return "UNSUPPORTED";
  case NOVA_ERR_FULL:
    return "FULL";
  default:
    return "UNKNOWN";
  }
}
