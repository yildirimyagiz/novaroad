#ifndef NOVA_MEMORY_PLANNER_H
#define NOVA_MEMORY_PLANNER_H

#include "compute/graph/nova_graph.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA MEMORY PLANNER - Buffer Reuse & Lifetime Analysis
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  size_t total_required;
  size_t peak_memory;
  int buffer_reuse_count;
} NovaMemoryPlan;

// Analyze a graph and create an optimal memory schedule
NovaMemoryPlan nova_memory_plan_graph(NovaExecGraph *graph);

#endif // NOVA_MEMORY_PLANNER_H
