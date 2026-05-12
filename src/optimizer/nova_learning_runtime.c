#include "../../include/nova_learning_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaLearningRuntime *nova_learning_init(const char *db_path) {
  NovaLearningRuntime *lr = calloc(1, sizeof(NovaLearningRuntime));
  lr->capacity = 1000;
  lr->knowledge_base = calloc(lr->capacity, sizeof(PerformancePattern));
  lr->db_path = strdup(db_path);

  nova_learning_load(lr);

  printf("📖 Nova Learning Runtime Online (Patterns: %zu)\n",
         lr->pattern_count);
  return lr;
}

void nova_learning_shutdown(NovaLearningRuntime *lr) {
  if (!lr)
    return;
  nova_learning_save(lr);
  free(lr->knowledge_base);
  free(lr->db_path);
  free(lr);
}

void nova_learning_observe(NovaLearningRuntime *lr, const uint8_t hash[32],
                             int strategy, double perf) {
  // Check if we already know this pattern
  for (size_t i = 0; i < lr->pattern_count; i++) {
    if (memcmp(lr->knowledge_base[i].code_hash, hash, 32) == 0) {
      // Update if performance is better
      if (perf > lr->knowledge_base[i].measured_performance) {
        lr->knowledge_base[i].measured_performance = perf;
        lr->knowledge_base[i].optimal_strategy_id = strategy;
      }
      return;
    }
  }

  // New pattern
  if (lr->pattern_count >= lr->capacity) {
    lr->capacity *= 2;
    lr->knowledge_base =
        realloc(lr->knowledge_base, lr->capacity * sizeof(PerformancePattern));
  }

  PerformancePattern *p = &lr->knowledge_base[lr->pattern_count++];
  memcpy(p->code_hash, hash, 32);
  p->measured_performance = perf;
  p->optimal_strategy_id = strategy;
}

int nova_learning_predict(NovaLearningRuntime *lr, const uint8_t hash[32]) {
  for (size_t i = 0; i < lr->pattern_count; i++) {
    if (memcmp(lr->knowledge_base[i].code_hash, hash, 32) == 0) {
      return lr->knowledge_base[i].optimal_strategy_id;
    }
  }
  return -1; // Unknown
}

void nova_learning_save(NovaLearningRuntime *lr) {
  FILE *f = fopen(lr->db_path, "wb");
  if (!f)
    return;
  fwrite(&lr->pattern_count, sizeof(size_t), 1, f);
  fwrite(lr->knowledge_base, sizeof(PerformancePattern), lr->pattern_count, f);
  fclose(f);
}

void nova_learning_load(NovaLearningRuntime *lr) {
  FILE *f = fopen(lr->db_path, "rb");
  if (!f)
    return;
  fread(&lr->pattern_count, sizeof(size_t), 1, f);
  fread(lr->knowledge_base, sizeof(PerformancePattern), lr->pattern_count, f);
  fclose(f);
}
