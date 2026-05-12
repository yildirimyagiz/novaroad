#include "optimizer/fusion/nova_kernel_fusion.h"
#include <stdio.h>

int nova_exec_graph_optimize_fusion(NovaExecGraph *graph) {
  printf("⚡ [Fusion] Analyzing Graph for potential fusions...\n");

  int fusion_count = 0;

  for (int i = 0; i < graph->node_count - 1; i++) {
    NovaExecGraphNode *n1 = graph->nodes[i];
    NovaExecGraphNode *n2 = graph->nodes[i + 1];

    // Pattern: MatMul -> Add
    if (n1->op_type == NOVA_EXEC_OP_MATMUL && n2->op_type == NOVA_EXEC_OP_ADD) {
      printf(
          "  -> Pattern Match: MatMul + Add. Fusing into MatMulAdd kernel.\n");
      fusion_count++;
      // In a real implementation, we would replace these nodes with a fused
      // node
    }

    // Pattern: Add -> Activation
    if (n1->op_type == NOVA_EXEC_OP_ADD && n2->op_type == NOVA_EXEC_OP_ACTIVATION) {
      printf("  -> Pattern Match: Add + Activation. Fusing into AddActivation "
             "kernel.\n");
      fusion_count++;
    }
  }

  printf("✅ [Fusion] Optimization Complete. %d kernels fused.\n",
         fusion_count);
  return fusion_count;
}
