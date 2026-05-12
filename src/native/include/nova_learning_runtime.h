#ifndef NOVA_LEARNING_RUNTIME_H
#define NOVA_LEARNING_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA PERSISTENT LEARNING RUNTIME
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  uint8_t code_hash[32];
  uint32_t context_id;
  double measured_performance;
  int optimal_strategy_id;
  int strategy_type; // Maps to NovaSearchStrategy
} PerformancePattern;

typedef struct NovaLearningRuntime {
  PerformancePattern *knowledge_base;
  size_t pattern_count;
  size_t capacity;

  // Bayesian Prior Data (Self-Learning)
  double global_mean_efficiency;
  uint64_t total_observations;

  char *db_path;
} NovaLearningRuntime;

NovaLearningRuntime *nova_learning_init(const char *db_path);
void nova_learning_shutdown(NovaLearningRuntime *lr);

// Learning API
void nova_learning_observe(NovaLearningRuntime *lr, const uint8_t hash[32],
                             int strategy, double perf);
int nova_learning_predict(NovaLearningRuntime *lr, const uint8_t hash[32]);

// Knowledge Management
void nova_learning_save(NovaLearningRuntime *lr);
void nova_learning_load(NovaLearningRuntime *lr);

#endif // NOVA_LEARNING_RUNTIME_H
