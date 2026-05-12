/**
 * Nova Runtime Profiler
 * Low-overhead runtime performance profiling
 */

#ifndef NOVA_RUNTIME_PROFILER_H
#define NOVA_RUNTIME_PROFILER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// PROFILER MODES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  PROFILE_DISABLED,       // No profiling (0% overhead)
  PROFILE_SAMPLING,       // Statistical sampling (~1% overhead)
  PROFILE_INSTRUMENTATION,// Full instrumentation (~5-10% overhead)
  PROFILE_MEMORY,         // Memory profiling
  PROFILE_CALLGRAPH,      // Call graph generation
} ProfileMode;

// ═══════════════════════════════════════════════════════════════════════════
// PROFILER DATA
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *function_name;
  uint64_t call_count;
  uint64_t total_time_ns;
  uint64_t self_time_ns;
  uint64_t min_time_ns;
  uint64_t max_time_ns;
  
  // Memory stats
  size_t allocated_bytes;
  size_t freed_bytes;
  size_t peak_memory;
  
  // Call graph
  struct ProfileEntry **callees;
  size_t callee_count;
} ProfileEntry;

typedef struct {
  ProfileMode mode;
  bool enabled;
  
  // Profile entries
  ProfileEntry *entries;
  size_t entry_count;
  size_t entry_capacity;
  
  // Call stack (for instrumentation)
  ProfileEntry **call_stack;
  size_t stack_depth;
  size_t stack_capacity;
  
  // Timing
  uint64_t *enter_times;
  
  // Statistics
  uint64_t total_samples;
  uint64_t profiling_overhead_ns;
  
  // Configuration
  uint32_t sampling_frequency_hz;
  bool track_memory;
  bool track_callgraph;
} RuntimeProfiler;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

// Lifecycle
RuntimeProfiler *profiler_create(ProfileMode mode);
void profiler_destroy(RuntimeProfiler *profiler);
void profiler_reset(RuntimeProfiler *profiler);

// Control
void profiler_enable(RuntimeProfiler *profiler);
void profiler_disable(RuntimeProfiler *profiler);
void profiler_set_mode(RuntimeProfiler *profiler, ProfileMode mode);
void profiler_set_sampling_frequency(RuntimeProfiler *profiler, uint32_t hz);

// Instrumentation
void profiler_enter_function(RuntimeProfiler *profiler, const char *name);
void profiler_exit_function(RuntimeProfiler *profiler);
void profiler_record_allocation(RuntimeProfiler *profiler, size_t bytes);
void profiler_record_deallocation(RuntimeProfiler *profiler, size_t bytes);

// Query
ProfileEntry *profiler_get_entry(RuntimeProfiler *profiler, const char *name);
ProfileEntry **profiler_get_top_functions(RuntimeProfiler *profiler, size_t count);
ProfileEntry **profiler_get_hotspots(RuntimeProfiler *profiler, size_t count);

// Reporting
void profiler_print_report(RuntimeProfiler *profiler);
void profiler_print_callgraph(RuntimeProfiler *profiler);
void profiler_print_memory_report(RuntimeProfiler *profiler);
char *profiler_export_json(RuntimeProfiler *profiler);
void profiler_save_report(RuntimeProfiler *profiler, const char *filename);

// ═══════════════════════════════════════════════════════════════════════════
// MACROS FOR EASY INSTRUMENTATION
// ═══════════════════════════════════════════════════════════════════════════

extern RuntimeProfiler *global_profiler;

#define PROFILE_FUNCTION_START(profiler, name) \
  do { if ((profiler) && (profiler)->enabled) profiler_enter_function(profiler, name); } while(0)

#define PROFILE_FUNCTION_END(profiler) \
  do { if ((profiler) && (profiler)->enabled) profiler_exit_function(profiler); } while(0)

#define PROFILE_SCOPE(profiler, name) \
  PROFILE_FUNCTION_START(profiler, name); \
  __attribute__((cleanup(profiler_scope_exit))) int __prof_guard_##name = 0

static inline void profiler_scope_exit(int *guard) {
  if (global_profiler && global_profiler->enabled) {
    profiler_exit_function(global_profiler);
  }
}

#endif // NOVA_RUNTIME_PROFILER_H
