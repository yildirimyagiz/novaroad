/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_graph_obligations.h — Graph Verification V2
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_GRAPH_OBLIGATIONS_H
#define NOVA_GRAPH_OBLIGATIONS_H

#include "nova_graph_structs.h"
#include "nova_obligation.h"

/**
 * V1 API — Returns SMT2-based ObligationSet for solver pipeline.
 * Used by nova_formal_graph_validate.
 */
ObligationSet *nova_graph_verify(const NovaIRGraph *graph);

/**
 * V2 API — Collects constraint-based obligations from all graph nodes.
 * Used for direct structural verification (no solver needed).
 */
void nova_graph_collect_obligations(const NovaIRGraph *g,
                                      NovaObligationList *out);

#endif /* NOVA_GRAPH_OBLIGATIONS_H */
