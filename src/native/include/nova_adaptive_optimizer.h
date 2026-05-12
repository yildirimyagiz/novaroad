#ifndef NOVA_ADAPTIVE_OPTIMIZER_H
#define NOVA_ADAPTIVE_OPTIMIZER_H

#include "nova_profiler_v2.h"
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ADAPTIVE OPTIMIZER
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  STRATEGY_SCALAR,
  STRATEGY_SIMD,
  STRATEGY_GPU,
  STRATEGY_DISTRIBUTED
} ExecutionStrategy;

typedef struct {
  char path_name[128];
  ExecutionStrategy current_strategy;
  uint32_t switch_count;
  double last_efficiency;
} AdaptiveOptimizationState;

typedef struct NovaAdaptiveOptimizer {
  AdaptiveOptimizationState *states;
  size_t state_count;
  size_t capacity;

  NovaProfilerV2 *profiler;
} NovaAdaptiveOptimizer;

NovaAdaptiveOptimizer *nova_adaptive_init(NovaProfilerV2 *profiler);
void nova_adaptive_shutdown(NovaAdaptiveOptimizer *ao);

// Decision logic
ExecutionStrategy nova_adaptive_select_strategy(NovaAdaptiveOptimizer *ao,
                                                  const char *path_name);
void nova_adaptive_feedback(NovaAdaptiveOptimizer *ao,
                              const char *path_name, bool success,
                              double metrics);

// Optimization loops
void nova_adaptive_run_loop(NovaAdaptiveOptimizer *ao);

#endif // NOVA_ADAPTIVE_OPTIMIZER_H
