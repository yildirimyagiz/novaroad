#ifndef NOVA_KERNEL_FUSION_H
#define NOVA_KERNEL_FUSION_H

#include "../../compute/graph/nova_graph.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA KERNEL FUSION - Pattern Matching & Op Merging
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Optimizes the graph by fusing compatible operations
int nova_exec_graph_optimize_fusion(NovaExecGraph *graph);

#endif // NOVA_KERNEL_FUSION_H
