#include "compute/graph/nova_graph.h"
#include "nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaGraph *nova_graph_create(NovaContext *ctx) {
  NovaGraph *graph = calloc(1, sizeof(NovaGraph));
  graph->ctx = ctx;
  graph->node_count = 0;
  graph->node_capacity = 64;
  graph->nodes = malloc(graph->node_capacity * sizeof(NovaGraphNode *));
  graph->is_finalized = false;
  graph->version = 1;
  return graph;
}

void nova_graph_destroy(NovaGraph *graph) {
  if (!graph)
    return;
  for (int i = 0; i < graph->node_count; i++) {
    NovaGraphNode *node = graph->nodes[i];
    if (node->inputs)
      free(node->inputs);
    if (node->outputs)
      free(node->outputs);
    if (node->name)
      free(node->name);
    // Note: Tensors might be shared or owned, handle with care.
    // Usually input tensors are NOT owned by graph.
    if (node->type == NOVA_NODE_OP && node->tensor) {
      nova_tensor_destroy(node->tensor);
    }
    free(node);
  }
  free(graph->nodes);
  free(graph);
}

NovaGraphNode *nova_graph_add_input(NovaGraph *graph, NovaTensor *t,
                                        const char *name) {
  if (graph->node_count >= graph->node_capacity) {
    graph->node_capacity *= 2;
    graph->nodes =
        realloc(graph->nodes, graph->node_capacity * sizeof(NovaGraphNode *));
  }

  NovaGraphNode *node = calloc(1, sizeof(NovaGraphNode));
  node->id = graph->node_count;
  node->type = NOVA_NODE_INPUT;
  node->tensor = t;
  node->name = strdup(name);

  graph->nodes[graph->node_count++] = node;
  return node;
}

NovaGraphNode *nova_graph_add_op(NovaGraph *graph, NovaGraphOpType op,
                                     NovaGraphNode **inputs, int input_count,
                                     const char *name) {
  if (graph->node_count >= graph->node_capacity) {
    graph->node_capacity *= 2;
    graph->nodes =
        realloc(graph->nodes, graph->node_capacity * sizeof(NovaGraphNode *));
  }

  NovaGraphNode *node = calloc(1, sizeof(NovaGraphNode));
  node->id = graph->node_count;
  node->type = NOVA_NODE_OP;
  node->op_type = op;
  node->input_count = input_count;
  node->inputs = malloc(input_count * sizeof(NovaGraphNode *));
  memcpy(node->inputs, inputs, input_count * sizeof(NovaGraphNode *));
  node->name = strdup(name);
  node->tensor = NULL;

  graph->nodes[graph->node_count++] = node;
  return node;
}

#include "nova_kernel_fusion.h"
#include "nova_kernels.h"

// ---- Graph VTable Adapter for Fusion Engine ----

static int64_t g_node_count(void *g) { return ((NovaGraph *)g)->node_count; }
static int64_t g_node_id_at(void *g, int64_t i) {
  return ((NovaGraph *)g)->nodes[i]->id;
}
static int32_t g_node_op(void *g, int64_t id) {
  NovaGraph *graph = (NovaGraph *)g;
  if (id >= 0 && id < graph->node_count) {
    // Only return valid op for actual OP nodes, not
    // inputs/constants/placeholders
    if (graph->nodes[id]->type == NOVA_NODE_OP)
      return graph->nodes[id]->op_type;
  }
  return -1;
}
static int32_t g_node_dtype(void *g, int64_t id) {
  (void)g;
  (void)id;
  // Assuming all float32 for MVP
  return 1; // NOVA_DTYPE_FP32
}
static int64_t g_node_input_count(void *g, int64_t id) {
  NovaGraph *graph = (NovaGraph *)g;
  if (id >= 0 && id < graph->node_count)
    return graph->nodes[id]->input_count;
  return 0;
}
static int64_t g_node_input(void *g, int64_t id, int64_t idx) {
  NovaGraph *graph = (NovaGraph *)g;
  if (id >= 0 && id < graph->node_count)
    return graph->nodes[id]->inputs[idx]->id;
  return -1;
}

static int64_t g_node_output_count(void *g, int64_t id) {
  // Simplified: Linear scan to find who uses this node. O(N) but OK for MVP.
  NovaGraph *graph = (NovaGraph *)g;
  int count = 0;
  for (int i = 0; i < graph->node_count; i++) {
    NovaGraphNode *n = graph->nodes[i];
    if (n->type != NOVA_NODE_OP)
      continue;
    for (int j = 0; j < n->input_count; j++) {
      if (n->inputs[j]->id == (uint64_t)id)
        count++;
    }
  }
  return count;
}

static int64_t g_node_output(void *g, int64_t id, int64_t idx) {
  NovaGraph *graph = (NovaGraph *)g;
  int current = 0;
  for (int i = 0; i < graph->node_count; i++) {
    NovaGraphNode *n = graph->nodes[i];
    if (n->type != NOVA_NODE_OP)
      continue;
    for (int j = 0; j < n->input_count; j++) {
      if (n->inputs[j]->id == (uint64_t)id) {
        if (current == idx)
          return n->id;
        current++;
      }
    }
  }
  return -1;
}

static int64_t g_node_create(void *g, int32_t op, const int64_t *inputs,
                             int64_t n_inputs) {
  NovaGraph *graph = (NovaGraph *)g;
  // inputs array contains IDs. Need to map back to pointers.
  NovaGraphNode **input_ptrs = malloc(n_inputs * sizeof(NovaGraphNode *));

  // The last input for FUSED op is the fused op ID, strictly speaking it's a
  // param not input node. But our adapter API passed it as input. The fusion
  // engine convention: last 'input' is param if Op=FUSED.
  int real_input_count = (op == NOVA_OP_FUSED) ? n_inputs - 1 : n_inputs;
  int64_t fused_op_id = (op == NOVA_OP_FUSED) ? inputs[n_inputs - 1] : 0;

  for (int i = 0; i < real_input_count; i++) {
    // Linear lookup for MVP.
    // Ideally use a hash map or direct indexing if ID == index (it is mostly).
    if (inputs[i] < graph->node_count) {
      input_ptrs[i] = graph->nodes[inputs[i]];
    }
  }

  NovaGraphNode *new_node = nova_graph_add_op(
      graph, (NovaGraphOpType)op, input_ptrs, real_input_count, "FusedNode");
  if (op == NOVA_OP_FUSED) {
    // Store the specific fused op code in params or piggyback
    // For MVP, just cast params to void* generic container or allocate int
    int *p = malloc(sizeof(int));
    *p = (int)fused_op_id;
    new_node->params = p;
  }

  free(input_ptrs);
  return new_node->id;
}

static void g_node_replace_uses(void *g, int64_t old_id, int64_t new_id) {
  NovaGraph *graph = (NovaGraph *)g;
  NovaGraphNode *new_node = NULL;

  // Find new node ptr
  if (new_id < graph->node_count &&
      graph->nodes[new_id]->id == (uint64_t)new_id) {
    new_node = graph->nodes[new_id];
  }

  if (!new_node)
    return;

  for (int i = 0; i < graph->node_count; i++) {
    NovaGraphNode *n = graph->nodes[i];
    if (n->type != NOVA_NODE_OP)
      continue;
    for (int j = 0; j < n->input_count; j++) {
      if (n->inputs[j]->id == (uint64_t)old_id) {
        n->inputs[j] = new_node;
      }
    }
  }
}

static void g_node_delete(void *g, int64_t id) {
  NovaGraph *graph = (NovaGraph *)g;
  // Soft delete: Mark as NO-OP or remove from execution list.
  // Real deletion complexity is high with array shifting.
  // Just mark op_type as CUSTOM and name as "DELETED" for now.
  if (id < graph->node_count) {
    // Hack: Change type to something innocuous that won't execute or cause
    // issues. Or just ensure executor checks 'is_executed' or similar. We will
    // introduce NOVA_NODE_DELETED type or just skip in executor loops.
    // Simplest: Set op type to -1 (invalid) and executor skips.
    // But we don't have invalid enum.
    // Let's use is_executed = true to skip it if it logic allows, but dag might
    // traverse. Better: Remove from `nodes` array? No indices shift. Let's mark
    // as constant/placeholder so it's ignored by executor logic focused on OPs.
    graph->nodes[id]->type = NOVA_NODE_PLACEHOLDER;
  }
}

static void g_node_set_tag(void *g, int64_t id, const char *tag) {
  NovaGraph *graph = (NovaGraph *)g;
  if (id < graph->node_count) {
    if (graph->nodes[id]->name)
      free(graph->nodes[id]->name);
    graph->nodes[id]->name = strdup(tag);
  }
}

static NovaGraphVTable g_vt = {.node_count = g_node_count,
                                 .node_id_at = g_node_id_at,
                                 .node_op = g_node_op,
                                 .node_dtype = g_node_dtype,
                                 .node_input_count = g_node_input_count,
                                 .node_input = g_node_input,
                                 .node_output_count = g_node_output_count,
                                 .node_output = g_node_output,
                                 .node_create = g_node_create,
                                 .node_replace_uses = g_node_replace_uses,
                                 .node_delete = g_node_delete,
                                 .node_set_tag = g_node_set_tag};

int nova_graph_compile(NovaGraph *graph) {
  // 1. Run Fusion Pass
  nova_fusion_init();
  int64_t fused = nova_fusion_run(graph, &g_vt);

  if (fused > 0) {
    printf("[Nova] Graph Compiled: %lld kernels fused (Optimized)\n", fused);
  } else {
    printf("[Nova] Graph Compiled: No fusions applied.\n");
  }

  graph->is_finalized = true;
  return 0;
}

int nova_graph_execute(NovaGraph *graph) {
  for (int i = 0; i < graph->node_count; i++) {
    NovaGraphNode *node = graph->nodes[i];

    // Skip deleted/placeholder nodes
    if (node->type == NOVA_NODE_PLACEHOLDER)
      continue;

    if (node->type == NOVA_NODE_OP && !node->is_executed) {

      switch (node->op_type) {
      case NOVA_OP_ADD:
        if (node->input_count == 2) {
          node->tensor = nova_tensor_add(node->inputs[0]->tensor,
                                           node->inputs[1]->tensor);
        }
        break;
      case NOVA_OP_MATMUL:
        if (node->input_count == 2) {
          node->tensor = nova_tensor_matmul(node->inputs[0]->tensor,
                                              node->inputs[1]->tensor);
        }
        break;
      case NOVA_OP_MUL:
        if (node->input_count == 2) {
          node->tensor = nova_tensor_mul(node->inputs[0]->tensor,
                                           node->inputs[1]->tensor);
        }
        break;
      case NOVA_OP_ACTIVATION:
        // Support ReLU for now
        node->tensor = nova_tensor_relu(node->inputs[0]->tensor);
        break;
      case NOVA_OP_FUSED: {
        int fused_id = *(int *)node->params;
        if (fused_id == NOVA_FUSED_MATMUL_ADD_RELU_F32) {
          NovaTensor *A = node->inputs[0]->tensor;
          NovaTensor *B = node->inputs[1]->tensor;
          NovaTensor *Bias = node->inputs[2]->tensor;

          int64_t M = A->shape[0];
          int64_t N = B->shape[1];
          int64_t out_shape[2] = {M, N};
          NovaTensor *out =
              nova_tensor_create(A->context, out_shape, 2, A->dtype);

          nova_kernel_matmul_add_relu_f32(A, B, Bias, out);
          out->last_kernel_name = "matmul_add_relu_f32";
          node->tensor = out;
        } else if (fused_id == NOVA_FUSED_CONV2D_BIAS_RELU_F32) {
          NovaTensor *X = node->inputs[0]->tensor;
          NovaTensor *W = node->inputs[1]->tensor;
          NovaTensor *Bias = node->inputs[2]->tensor;

          // Assume NCHW. Output shape: [N, OC, OH, OW]
          // OH = (H + 2P - KH)/S + 1
          // For now, assume SAME padding (or no padding for simplicity if not
          // in node)
          int64_t batch = X->shape[0];
          int64_t oc = W->shape[0];
          int64_t oh =
              X->shape[2]; // Simplified: assume 1x1 or stride=1 same pad
          int64_t ow = X->shape[3];

          int64_t out_shape[4] = {batch, oc, oh, ow};
          NovaTensor *out =
              nova_tensor_create(X->context, out_shape, 4, X->dtype);

          nova_kernel_conv2d_bias_relu_f32(X, W, Bias, out);
          out->last_kernel_name = "conv2d_bias_relu_f32";
          node->tensor = out;
        }
        break;
      }
      default:
        printf("  ⚠️ Warning: Op %d not yet implemented in graph executor\n",
               node->op_type);
        break;
      }

      node->is_executed = true;
    }
  }

  return 0;
}

NovaTensor *nova_graph_node_get_tensor(NovaGraphNode *node) {
  return node ? node->tensor : NULL;
}
