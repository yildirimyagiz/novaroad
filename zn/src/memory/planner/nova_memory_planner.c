#include "memory/planner/nova_memory_planner.h"
#include <stdio.h>

NovaMemoryPlan nova_memory_plan_graph(NovaGraph *graph) {
  printf("🧠 [MemoryPlanner] Planning memory for Graph with %d nodes...\n",
         graph->node_count);

  NovaMemoryPlan plan = {0, 0, 0};

  // 1. Calculate Birth/Death of each tensor
  // 2. Greedy algorithm for buffer assignment
  // 3. Offset calculation for a single large arena

  for (int i = 0; i < graph->node_count; i++) {
    NovaGraphNode *node = graph->nodes[i];
    if (node->type == NOVA_NODE_OP) {
      // Estimate size and assign offset
      node->scheduled_offset = plan.total_required;
      plan.total_required += (1024 * 1024); // Dummy 1MB increments
    }
  }

  plan.peak_memory = plan.total_required;
  printf("✅ [MemoryPlanner] Plan Complete. Peak Memory: %zu bytes\n",
         plan.peak_memory);

  return plan;
}
