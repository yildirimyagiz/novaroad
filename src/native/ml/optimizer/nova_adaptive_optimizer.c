#include "../../include/nova_adaptive_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaAdaptiveOptimizer *nova_adaptive_init(NovaProfilerV2 *profiler) {
  NovaAdaptiveOptimizer *ao = calloc(1, sizeof(NovaAdaptiveOptimizer));
  ao->capacity = 256;
  ao->states = calloc(ao->capacity, sizeof(AdaptiveOptimizationState));
  ao->profiler = profiler;

  printf("🧬 Nova Adaptive Optimizer Initialized (Linked to Profiler V2)\n");
  return ao;
}

void nova_adaptive_shutdown(NovaAdaptiveOptimizer *ao) {
  if (!ao)
    return;
  free(ao->states);
  free(ao);
}

static AdaptiveOptimizationState *get_state(NovaAdaptiveOptimizer *ao,
                                            const char *path_name) {
  for (size_t i = 0; i < ao->state_count; i++) {
    if (strcmp(ao->states[i].path_name, path_name) == 0) {
      return &ao->states[i];
    }
  }

  if (ao->state_count >= ao->capacity) {
    ao->capacity *= 2;
    ao->states =
        realloc(ao->states, ao->capacity * sizeof(AdaptiveOptimizationState));
  }

  AdaptiveOptimizationState *state = &ao->states[ao->state_count++];
  strncpy(state->path_name, path_name, 127);
  state->current_strategy = STRATEGY_SCALAR;
  state->switch_count = 0;
  return state;
}

ExecutionStrategy nova_adaptive_select_strategy(NovaAdaptiveOptimizer *ao,
                                                  const char *path_name) {
  if (!ao)
    return STRATEGY_SCALAR;

  AdaptiveOptimizationState *state = get_state(ao, path_name);

  // Logic: If hot path detected by profiler, try SIMD. If still hot, try GPU.
  bool is_hot = nova_profiler_is_hot(ao->profiler, path_name);

  if (is_hot) {
    if (state->current_strategy == STRATEGY_SCALAR) {
      state->current_strategy = STRATEGY_SIMD;
      state->switch_count++;
      printf("✨ %s: Hot path detected! Switching to SIMD\n", path_name);
    } else if (state->current_strategy == STRATEGY_SIMD &&
               state->switch_count > 5) {
      // If it's still hot after many SIMD runs, maybe GPU is better
      state->current_strategy = STRATEGY_GPU;
      state->switch_count++;
      printf("🚀 %s: Extremely hot path! Offloading to GPU\n", path_name);
    }
  } else {
    // If it cooled down, maybe go back to scalar to save energy/resources
    if (state->current_strategy != STRATEGY_SCALAR) {
      // state->current_strategy = STRATEGY_SCALAR;
      // printf("🧊 %s: Cooled down. Reverting to Scalar\n", path_name);
    }
  }

  return state->current_strategy;
}

void nova_adaptive_feedback(NovaAdaptiveOptimizer *ao,
                              const char *path_name, bool success,
                              double metrics) {
  if (!ao)
    return;
  AdaptiveOptimizationState *state = get_state(ao, path_name);
  state->last_efficiency = metrics;

  if (!success) {
    // Fallback strategy
    state->current_strategy = STRATEGY_SCALAR;
    printf("⚠️ %s: Optimization failure. Rolling back to Scalar.\n", path_name);
  }
}

void nova_adaptive_run_loop(NovaAdaptiveOptimizer *ao) {
  // Process feedback loop
  // This would be called by a background thread or at safe intervals
}
