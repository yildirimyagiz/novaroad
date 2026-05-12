#ifndef NOVA_COGNITIVE_SCHEDULER_H
#define NOVA_COGNITIVE_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA COGNITIVE SCHEDULER
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  WORKLOAD_IO_BOUND,
  WORKLOAD_COMPUTE_BOUND,
  WORKLOAD_LATENCY_CRITICAL,
  WORKLOAD_THROUGHPUT_ORIENTED,
  WORKLOAD_AI_INFERENCE
} WorkloadType;

typedef struct {
  WorkloadType type;
  double priority;
  uint32_t expected_duration_us;
  uint32_t energy_budget_uj;
} WorkloadProfile;

typedef struct {
  char task_name[128];
  WorkloadProfile profile;
  void (*task_fn)(void *);
  void *arg;
} NovaTask;

typedef struct NovaCognitiveScheduler {
  NovaTask *task_queue;
  size_t queue_size;
  size_t capacity;

  // Cognitive metrics
  double system_load;
  double current_energy_draw;
  double thermal_headroom;
} NovaCognitiveScheduler;

NovaCognitiveScheduler *nova_scheduler_init(void);
void nova_scheduler_shutdown(NovaCognitiveScheduler *s);

// Scheduling API
void nova_scheduler_submit(NovaCognitiveScheduler *s, NovaTask task);
void nova_scheduler_plan(NovaCognitiveScheduler *s); // Predictive planning
void nova_scheduler_execute_next(NovaCognitiveScheduler *s);

// Balancing
void nova_scheduler_rebalance(NovaCognitiveScheduler *s);

#endif // NOVA_COGNITIVE_SCHEDULER_H
