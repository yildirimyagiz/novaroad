#include "../../include/nova_profiler_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

// Global profiler instance for compiler hooks
static NovaProfilerV2 *g_profiler = NULL;

static uint64_t get_nanos(void) {
#ifdef __APPLE__
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

NovaProfilerV2 *nova_profiler_init(void) {
  NovaProfilerV2 *p = calloc(1, sizeof(NovaProfilerV2));
  p->capacity = 1024;
  p->nodes = calloc(p->capacity, sizeof(NovaProfilerNode));
  p->sampling_enabled = true;
  p->sampling_rate = 100; // Sample 1% by default for low overhead

  g_profiler = p;
  printf("🚀 Nova Profiler V2 Initialized (Sampling: %u:1)\n",
         p->sampling_rate);
  return p;
}

void nova_profiler_shutdown(NovaProfilerV2 *p) {
  if (!p)
    return;
  nova_profiler_dump_stats(p);
  free(p->nodes);
  free(p);
  g_profiler = NULL;
}

static NovaProfilerNode *get_node(NovaProfilerV2 *p, const char *name) {
  // Simple linear search for now, hash table would be better for high node
  // counts
  for (size_t i = 0; i < p->node_count; i++) {
    if (strcmp(p->nodes[i].name, name) == 0) {
      return &p->nodes[i];
    }
  }

  if (p->node_count >= p->capacity) {
    p->capacity *= 2;
    p->nodes = realloc(p->nodes, p->capacity * sizeof(NovaProfilerNode));
  }

  NovaProfilerNode *node = &p->nodes[p->node_count++];
  strncpy(node->name, name, 127);
  return node;
}

void nova_profiler_begin(NovaProfilerV2 *p, const char *path_name) {
  if (!p || (p->sampling_enabled && (rand() % p->sampling_rate != 0)))
    return;

  NovaProfilerNode *node = get_node(p, path_name);
  node->metrics[METRIC_CPU_CYCLES] = get_nanos();
}

void nova_profiler_end(NovaProfilerV2 *p, const char *path_name) {
  if (!p)
    return;

  uint64_t end_time = get_nanos();
  NovaProfilerNode *node = get_node(p, path_name);

  // Check if we were measuring this sample
  if (node->metrics[METRIC_CPU_CYCLES] == 0)
    return;

  uint64_t duration = end_time - node->metrics[METRIC_CPU_CYCLES];
  node->metrics[METRIC_CPU_CYCLES] = 0; // Reset for next use

  double dur_ns = (double)duration;
  NovaRuntimeStats *s = &node->stats;

  if (s->count == 0) {
    s->min_ns = dur_ns;
    s->max_ns = dur_ns;
    s->avg_ns = dur_ns;
  } else {
    if (dur_ns < s->min_ns)
      s->min_ns = dur_ns;
    if (dur_ns > s->max_ns)
      s->max_ns = dur_ns;
    s->avg_ns = (s->avg_ns * s->count + dur_ns) / (s->count + 1);
  }
  s->count++;

  // Hot path detection: if average * count is high relative to others
  if (s->avg_ns * s->count >
      10000000) { // arbitrary threshold: 10ms total execution
    node->is_hot_path = true;
  }
}

void __nova_profiler_hook_enter(const char *fn_name) {
  nova_profiler_begin(g_profiler, fn_name);
}

void __nova_profiler_hook_exit(const char *fn_name) {
  nova_profiler_end(g_profiler, fn_name);
}

void nova_profiler_dump_stats(NovaProfilerV2 *p) {
  if (!p)
    return;

  printf("\n📊 NOVA RUNTIME PROFILE REPORT\n");
  printf("---------------------------------------------------------------------"
         "-----------\n");
  printf("%-32s %-10s %-12s %-12s %-12s %-5s\n", "Path/Function", "Samples",
         "Avg(ns)", "Min(ns)", "Max(ns)", "Hot");
  printf("---------------------------------------------------------------------"
         "-----------\n");

  for (size_t i = 0; i < p->node_count; i++) {
    NovaProfilerNode *n = &p->nodes[i];
    printf("%-32s %-10llu %-12.2f %-12.2f %-12.2f %-5s\n", n->name,
           (unsigned long long)n->stats.count, n->stats.avg_ns, n->stats.min_ns,
           n->stats.max_ns, n->is_hot_path ? "🔥" : "");
  }
  printf("---------------------------------------------------------------------"
         "-----------\n\n");
}

void nova_profiler_set_sampling(NovaProfilerV2 *p, bool enabled,
                                  uint32_t rate) {
  if (!p)
    return;
  p->sampling_enabled = enabled;
  p->sampling_rate = rate;
}

bool nova_profiler_is_hot(NovaProfilerV2 *p, const char *path_name) {
  if (!p)
    return false;
  for (size_t i = 0; i < p->node_count; i++) {
    if (strcmp(p->nodes[i].name, path_name) == 0) {
      return p->nodes[i].is_hot_path;
    }
  }
  return false;
}
