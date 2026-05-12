#include "../../include/nova_cognitive_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaCognitiveScheduler *nova_scheduler_init(void) {
  NovaCognitiveScheduler *s = calloc(1, sizeof(NovaCognitiveScheduler));
  s->capacity = 1024;
  s->task_queue = calloc(s->capacity, sizeof(NovaTask));
  s->thermal_headroom = 100.0;

  printf(
      "🧠 Nova Cognitive Scheduler Initialized (Thermal Headroom: %.1f%%)\n",
      s->thermal_headroom);
  return s;
}

void nova_scheduler_shutdown(NovaCognitiveScheduler *s) {
  if (!s)
    return;
  free(s->task_queue);
  free(s);
}

void nova_scheduler_submit(NovaCognitiveScheduler *s, NovaTask task) {
  if (s->queue_size >= s->capacity) {
    s->capacity *= 2;
    s->task_queue = realloc(s->task_queue, s->capacity * sizeof(NovaTask));
  }

  s->task_queue[s->queue_size++] = task;

  // Auto-replan after submission if critical
  if (task.profile.type == WORKLOAD_LATENCY_CRITICAL) {
    nova_scheduler_plan(s);
  }
}

static int compare_tasks(const void *a, const void *b) {
  const NovaTask *ta = (const NovaTask *)a;
  const NovaTask *tb = (const NovaTask *)b;

  // Priority 1: Latency critical tasks first
  if (ta->profile.type == WORKLOAD_LATENCY_CRITICAL &&
      tb->profile.type != WORKLOAD_LATENCY_CRITICAL)
    return -1;
  if (ta->profile.type != WORKLOAD_LATENCY_CRITICAL &&
      tb->profile.type == WORKLOAD_LATENCY_CRITICAL)
    return 1;

  // Priority 2: Higher user-defined priority
  if (ta->profile.priority > tb->profile.priority)
    return -1;
  if (ta->profile.priority < tb->profile.priority)
    return 1;

  return 0;
}

void nova_scheduler_plan(NovaCognitiveScheduler *s) {
  if (s->queue_size < 2)
    return;

  // Sort tasks based on cognitive heuristics
  qsort(s->task_queue, s->queue_size, sizeof(NovaTask), compare_tasks);

  printf("🔮 Scheduler: Planned %zu tasks (Top task: %s, Priority: %.2f)\n",
         s->queue_size, s->task_queue[0].task_name,
         s->task_queue[0].profile.priority);
}

void nova_scheduler_execute_next(NovaCognitiveScheduler *s) {
  if (s->queue_size == 0)
    return;

  NovaTask t = s->task_queue[0];

  // Shift queue
  memmove(&s->task_queue[0], &s->task_queue[1],
          (s->queue_size - 1) * sizeof(NovaTask));
  s->queue_size--;

  // Execute
  if (t.task_fn) {
    t.task_fn(t.arg);
  }
}

void nova_scheduler_rebalance(NovaCognitiveScheduler *s) {
  // Dynamically adjust load based on thermal/energy metrics
  if (s->thermal_headroom < 20.0) {
    printf(
        "🌡️ Thermal throttling active! De-prioritizing heavy compute tasks.\n");
    // In a real implementation, we'd adjust priorities of COMPUTE_BOUND tasks
    // here
  }
}
