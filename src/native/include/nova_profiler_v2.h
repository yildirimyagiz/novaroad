#ifndef NOVA_PROFILER_V2_H
#define NOVA_PROFILER_V2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA UNIFIED PROFILER V2
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  METRIC_CPU_CYCLES,
  METRIC_INSTRUCTIONS,
  METRIC_CACHE_MISSES,
  METRIC_BRANCH_MISSES,
  METRIC_SIMD_UTILIZATION,
  METRIC_GPU_KERNELS,
  METRIC_MEMORY_BANDWIDTH,
  METRIC_ENERGY_CONSUMPTION
} NovaMetricType;

typedef struct {
  uint64_t count;
  double min_ns;
  double max_ns;
  double avg_ns;
  double variance;
} NovaRuntimeStats;

typedef struct {
  char name[128];
  NovaRuntimeStats stats;
  uint64_t metrics[8]; // Indexed by NovaMetricType
  bool is_hot_path;
} NovaProfilerNode;

typedef struct NovaProfilerV2 {
  NovaProfilerNode *nodes;
  size_t node_count;
  size_t capacity;

  // Sampling configuration
  bool sampling_enabled;
  uint32_t sampling_rate; // 1 in N
  uint64_t total_samples;

  // Hardware counters
  int perf_fd;
  bool hw_counters_supported;
} NovaProfilerV2;

// Core API
NovaProfilerV2 *nova_profiler_init(void);
void nova_profiler_shutdown(NovaProfilerV2 *p);

// Manual Hooks (for critical sections)
void nova_profiler_begin(NovaProfilerV2 *p, const char *path_name);
void nova_profiler_end(NovaProfilerV2 *p, const char *path_name);

// Automated Hooks (to be injected by compiler)
void __nova_profiler_hook_enter(const char *fn_name);
void __nova_profiler_hook_exit(const char *fn_name);

// Metrics & Analytics
void nova_profiler_log_metric(NovaProfilerV2 *p, NovaMetricType type,
                                uint64_t value);
void nova_profiler_dump_stats(NovaProfilerV2 *p);
bool nova_profiler_is_hot(NovaProfilerV2 *p, const char *path_name);

// Adaptive Control
void nova_profiler_set_sampling(NovaProfilerV2 *p, bool enabled,
                                  uint32_t rate);

#endif // NOVA_PROFILER_V2_H
