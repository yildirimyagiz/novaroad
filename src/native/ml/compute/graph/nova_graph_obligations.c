/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_graph_obligations.c — Graph-wide Consistency Proofs
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Compute Graph'ı dolaşır ve her düğüm (node) ve kenar (edge) için
 * tutarlılık kanıtları (consistency obligations) üretir.
 *
 * V1: SMT2-string based ObligationSet (for solver pipeline)
 * V2: Constraint-based NovaObligationList (direct structural check)
 */

#include "nova_graph_obligations.h"
#include "nova_kernel_contracts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * V1 API — SMT2 Solver Pipeline
 * ═══════════════════════════════════════════════════════════════════════════
 */

ObligationSet *nova_graph_verify(const NovaIRGraph *graph) {
  if (!graph)
    return NULL;

  ObligationSet *set = obligation_set_create("GraphVerification");

  /* 1. Global Resource Check */
  {
    uint64_t total_mem = 0;
    for (uint32_t i = 0; i < graph->num_nodes; i++) {
      NovaIRNode *n = graph->nodes[i];
      if (n)
        total_mem += n->output_size_bytes;
    }

    char mem_formula[256];
    snprintf(mem_formula, sizeof(mem_formula), "(assert (<= %llu %llu))",
             (unsigned long long)total_mem,
             (unsigned long long)get_device_memory_limit());

    obligation_set_add(set, OBL_RESOURCE_BOUND, "Global Graph VRAM Limit Check",
                       strdup(mem_formula));
  }

  /* 2. Edge Consistency */
  for (uint32_t i = 0; i < graph->num_nodes; i++) {
    NovaIRNode *src = graph->nodes[i];
    if (!src)
      continue;

    for (int j = 0; j < src->num_output_edges; j++) {
      NovaIREdge *edge = src->output_edges[j];
      if (!edge)
        continue;

      NovaIRNode *dst = edge->target_node;
      if (!dst)
        continue;

      if (edge->target_input_idx >= 0 &&
          edge->target_input_idx < dst->num_input_edges) {
        if (src->output_types && dst->input_types &&
            src->output_types[j] != dst->input_types[edge->target_input_idx]) {
          obligation_set_add(set, OBL_TYPE_SAFE,
                             "Edge Type Mismatch (Src -> Dst)",
                             "(assert false)");
        }
      }
    }
  }

  /* 3. Node-local contracts (V2 bridge) */
  for (uint32_t i = 0; i < graph->num_nodes; i++) {
    NovaIRNode *n = graph->nodes[i];
    if (!n)
      continue;

    NovaObligation ob = nova_contract_from_node(n);
    if (!ob.all_satisfied) {
      /* Convert V2 failure into V1 obligation */
      char desc[256];
      snprintf(desc, sizeof(desc), "Node '%s': contract violation",
               ob.op_name ? ob.op_name : "?");
      obligation_set_add(set, OBL_SHAPE_SAFE, strdup(desc), "(assert false)");
    }
  }

  return set;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V2 API — Direct Structural Verification
 * ═══════════════════════════════════════════════════════════════════════════
 */

static void emit_edge_obligations(const NovaIRGraph *g,
                                  NovaObligationList *out) {
  if (!g)
    return;
  NovaObligation ob = nova_obligation_begin("graph_topology");
  bool all_edges_ok = true;

  for (uint32_t i = 0; i < g->num_nodes; i++) {
    NovaIRNode *src = g->nodes[i];
    if (!src)
      continue;

    for (int j = 0; j < src->num_output_edges; j++) {
      NovaIREdge *edge = src->output_edges[j];
      if (!edge || !edge->target_node)
        continue;

      NovaIRNode *dst = edge->target_node;
      int dst_idx = edge->target_input_idx;

      if (dst_idx >= 0 && dst_idx < dst->num_input_edges) {
        if (src->output_types && dst->input_types) {
          if (src->output_types[j] != dst->input_types[dst_idx]) {
            all_edges_ok = false;
            /* We could add detailed message but V2 obligation stores bools per
             * req */
          }
        }
      }
    }
  }
  nova_obligation_require(&ob, "edges_type_consistent", all_edges_ok);
  nova_obligation_list_push(out, ob);
}

static void emit_resource_obligations(const NovaIRGraph *g,
                                      NovaObligationList *out) {
  if (!g)
    return;
  NovaObligation ob = nova_obligation_begin("graph_resources");

  uint64_t total_mem = 0;
  for (uint32_t i = 0; i < g->num_nodes; i++) {
    if (g->nodes[i]) {
      total_mem += g->nodes[i]->output_size_bytes;
    }
  }

  /* Hardcoded 1GB limit for V2 prototype, should fetch from device config */
  uint64_t limit = 1024ULL * 1024 * 1024;
  nova_obligation_require(&ob, "vram_limit_safe", total_mem <= limit);

  nova_obligation_list_push(out, ob);
}

static void emit_node_local_obligations(const NovaIRGraph *g,
                                        NovaObligationList *out) {
  for (uint32_t i = 0; i < g->num_nodes; i++) {
    const NovaIRNode *n = g->nodes[i];
    NovaObligation ob = nova_contract_from_node(n);
    nova_obligation_list_push(out, ob);
  }
}

void nova_graph_collect_obligations(const NovaIRGraph *g,
                                      NovaObligationList *out) {
  if (!g || !out)
    return;
  emit_resource_obligations(g, out);
  emit_edge_obligations(g, out);
  emit_node_local_obligations(g, out);

  /* Set verification status on graph for backend optimization */
  if (g && out) {
    bool passed = nova_obligation_list_all_satisfied(out);
    /* Cast away constness to update metadata flags */
    NovaIRGraph *mutable_g = (NovaIRGraph *)g;
    mutable_g->is_verified = passed;
    /* If passed resource checks, memory is proven safe */
    mutable_g->is_memory_proven = passed;
  }
}
