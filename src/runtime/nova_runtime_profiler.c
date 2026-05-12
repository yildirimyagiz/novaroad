/**
 * Nova Runtime Profiler Implementation
 */

#include "nova_runtime_profiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef __linux__
#include <sys/time.h>
#endif

RuntimeProfiler *global_profiler = NULL;

// ═══════════════════════════════════════════════════════════════════════════
// TIME UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t get_time_nanoseconds(void) {
#ifdef __linux__
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#else
  return clock() * 1000000000ULL / CLOCKS_PER_SEC;
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

RuntimeProfiler *profiler_create(ProfileMode mode) {
  RuntimeProfiler *profiler = (RuntimeProfiler *)calloc(1, sizeof(RuntimeProfiler));
  if (!profiler) return NULL;
  
  profiler->mode = mode;
  profiler->enabled = (mode != PROFILE_DISABLED);
  
  profiler->entry_capacity = 1024;
  profiler->entries = (ProfileEntry *)calloc(profiler->entry_capacity, sizeof(ProfileEntry));
  
  profiler->stack_capacity = 256;
  profiler->call_stack = (ProfileEntry **)malloc(profiler->stack_capacity * sizeof(ProfileEntry*));
  profiler->enter_times = (uint64_t *)malloc(profiler->stack_capacity * sizeof(uint64_t));
  
  profiler->sampling_frequency_hz = 1000;
  profiler->track_memory = (mode == PROFILE_MEMORY);
  profiler->track_callgraph = (mode == PROFILE_CALLGRAPH);
  
  if (!global_profiler) {
    global_profiler = profiler;
  }
  
  return profiler;
}

void profiler_destroy(RuntimeProfiler *profiler) {
  if (!profiler) return;
  
  for (size_t i = 0; i < profiler->entry_count; i++) {
    free(profiler->entries[i].function_name);
    free(profiler->entries[i].callees);
  }
  free(profiler->entries);
  free(profiler->call_stack);
  free(profiler->enter_times);
  
  if (global_profiler == profiler) {
    global_profiler = NULL;
  }
  
  free(profiler);
}

void profiler_reset(RuntimeProfiler *profiler) {
  if (!profiler) return;
  
  for (size_t i = 0; i < profiler->entry_count; i++) {
    free(profiler->entries[i].function_name);
    free(profiler->entries[i].callees);
  }
  
  profiler->entry_count = 0;
  profiler->stack_depth = 0;
  profiler->total_samples = 0;
  profiler->profiling_overhead_ns = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// CONTROL
// ═══════════════════════════════════════════════════════════════════════════

void profiler_enable(RuntimeProfiler *profiler) {
  if (profiler) profiler->enabled = true;
}

void profiler_disable(RuntimeProfiler *profiler) {
  if (profiler) profiler->enabled = false;
}

void profiler_set_mode(RuntimeProfiler *profiler, ProfileMode mode) {
  if (!profiler) return;
  profiler->mode = mode;
  profiler->enabled = (mode != PROFILE_DISABLED);
}

void profiler_set_sampling_frequency(RuntimeProfiler *profiler, uint32_t hz) {
  if (profiler) profiler->sampling_frequency_hz = hz;
}

// ═══════════════════════════════════════════════════════════════════════════
// ENTRY MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

static ProfileEntry *find_or_create_entry(RuntimeProfiler *profiler, const char *name) {
  // Find existing
  for (size_t i = 0; i < profiler->entry_count; i++) {
    if (strcmp(profiler->entries[i].function_name, name) == 0) {
      return &profiler->entries[i];
    }
  }
  
  // Create new
  if (profiler->entry_count >= profiler->entry_capacity) {
    profiler->entry_capacity *= 2;
    profiler->entries = (ProfileEntry *)realloc(profiler->entries,
                                                 profiler->entry_capacity * sizeof(ProfileEntry));
  }
  
  ProfileEntry *entry = &profiler->entries[profiler->entry_count++];
  memset(entry, 0, sizeof(ProfileEntry));
  entry->function_name = strdup(name);
  entry->min_time_ns = UINT64_MAX;
  
  return entry;
}

// ═══════════════════════════════════════════════════════════════════════════
// INSTRUMENTATION
// ═══════════════════════════════════════════════════════════════════════════

void profiler_enter_function(RuntimeProfiler *profiler, const char *name) {
  if (!profiler || !profiler->enabled) return;
  
  uint64_t start = get_time_nanoseconds();
  
  ProfileEntry *entry = find_or_create_entry(profiler, name);
  entry->call_count++;
  
  // Push onto call stack
  if (profiler->stack_depth >= profiler->stack_capacity) {
    profiler->stack_capacity *= 2;
    profiler->call_stack = (ProfileEntry **)realloc(profiler->call_stack,
                                                      profiler->stack_capacity * sizeof(ProfileEntry*));
    profiler->enter_times = (uint64_t *)realloc(profiler->enter_times,
                                                  profiler->stack_capacity * sizeof(uint64_t));
  }
  
  profiler->call_stack[profiler->stack_depth] = entry;
  profiler->enter_times[profiler->stack_depth] = get_time_nanoseconds();
  profiler->stack_depth++;
  
  uint64_t end = get_time_nanoseconds();
  profiler->profiling_overhead_ns += (end - start);
}

void profiler_exit_function(RuntimeProfiler *profiler) {
  if (!profiler || !profiler->enabled || profiler->stack_depth == 0) return;
  
  uint64_t start = get_time_nanoseconds();
  
  profiler->stack_depth--;
  ProfileEntry *entry = profiler->call_stack[profiler->stack_depth];
  uint64_t enter_time = profiler->enter_times[profiler->stack_depth];
  
  uint64_t duration = start - enter_time;
  
  entry->total_time_ns += duration;
  entry->self_time_ns += duration;
  
  if (duration < entry->min_time_ns) entry->min_time_ns = duration;
  if (duration > entry->max_time_ns) entry->max_time_ns = duration;
  
  // Update call graph
  if (profiler->track_callgraph && profiler->stack_depth > 0) {
    ProfileEntry *caller = profiler->call_stack[profiler->stack_depth - 1];
    
    // Check if already in callees
    bool found = false;
    for (size_t i = 0; i < caller->callee_count; i++) {
      if (caller->callees[i] == entry) {
        found = true;
        break;
      }
    }
    
    if (!found) {
      caller->callees = (ProfileEntry **)realloc(caller->callees,
                                                   (caller->callee_count + 1) * sizeof(ProfileEntry*));
      caller->callees[caller->callee_count++] = entry;
    }
    
    // Subtract from caller's self time
    caller->self_time_ns -= duration;
  }
  
  uint64_t end = get_time_nanoseconds();
  profiler->profiling_overhead_ns += (end - start);
}

void profiler_record_allocation(RuntimeProfiler *profiler, size_t bytes) {
  if (!profiler || !profiler->enabled || !profiler->track_memory) return;
  if (profiler->stack_depth == 0) return;
  
  ProfileEntry *entry = profiler->call_stack[profiler->stack_depth - 1];
  entry->allocated_bytes += bytes;
  
  size_t current = entry->allocated_bytes - entry->freed_bytes;
  if (current > entry->peak_memory) {
    entry->peak_memory = current;
  }
}

void profiler_record_deallocation(RuntimeProfiler *profiler, size_t bytes) {
  if (!profiler || !profiler->enabled || !profiler->track_memory) return;
  if (profiler->stack_depth == 0) return;
  
  ProfileEntry *entry = profiler->call_stack[profiler->stack_depth - 1];
  entry->freed_bytes += bytes;
}

// ═══════════════════════════════════════════════════════════════════════════
// QUERY
// ═══════════════════════════════════════════════════════════════════════════

ProfileEntry *profiler_get_entry(RuntimeProfiler *profiler, const char *name) {
  if (!profiler || !name) return NULL;
  
  for (size_t i = 0; i < profiler->entry_count; i++) {
    if (strcmp(profiler->entries[i].function_name, name) == 0) {
      return &profiler->entries[i];
    }
  }
  
  return NULL;
}

static int compare_total_time(const void *a, const void *b) {
  const ProfileEntry *ea = *(const ProfileEntry **)a;
  const ProfileEntry *eb = *(const ProfileEntry **)b;
  return (eb->total_time_ns > ea->total_time_ns) - (eb->total_time_ns < ea->total_time_ns);
}

ProfileEntry **profiler_get_top_functions(RuntimeProfiler *profiler, size_t count) {
  if (!profiler) return NULL;
  
  ProfileEntry **sorted = (ProfileEntry **)malloc(profiler->entry_count * sizeof(ProfileEntry*));
  for (size_t i = 0; i < profiler->entry_count; i++) {
    sorted[i] = &profiler->entries[i];
  }
  
  qsort(sorted, profiler->entry_count, sizeof(ProfileEntry*), compare_total_time);
  
  if (count > profiler->entry_count) count = profiler->entry_count;
  
  ProfileEntry **result = (ProfileEntry **)malloc(count * sizeof(ProfileEntry*));
  memcpy(result, sorted, count * sizeof(ProfileEntry*));
  free(sorted);
  
  return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// REPORTING
// ═══════════════════════════════════════════════════════════════════════════

void profiler_print_report(RuntimeProfiler *profiler) {
  if (!profiler) return;
  
  printf("\n=== Runtime Profile Report ===\n");
  printf("Mode: %s\n", profiler->mode == PROFILE_SAMPLING ? "Sampling" :
                       profiler->mode == PROFILE_INSTRUMENTATION ? "Instrumentation" :
                       profiler->mode == PROFILE_MEMORY ? "Memory" : "Unknown");
  printf("Total functions: %zu\n", profiler->entry_count);
  printf("Profiling overhead: %.3f ms\n\n", profiler->profiling_overhead_ns / 1e6);
  
  printf("%-40s %10s %12s %12s %12s\n",
         "Function", "Calls", "Total (ms)", "Self (ms)", "Avg (μs)");
  printf("%.100s\n", "----------------------------------------"
                     "----------------------------------------"
                     "--------------------");
  
  ProfileEntry **top = profiler_get_top_functions(profiler, 20);
  
  for (size_t i = 0; i < 20 && i < profiler->entry_count; i++) {
    ProfileEntry *e = top[i];
    double total_ms = e->total_time_ns / 1e6;
    double self_ms = e->self_time_ns / 1e6;
    double avg_us = e->call_count > 0 ? (e->total_time_ns / (double)e->call_count) / 1000.0 : 0.0;
    
    printf("%-40s %10llu %12.3f %12.3f %12.3f\n",
           e->function_name, (unsigned long long)e->call_count,
           total_ms, self_ms, avg_us);
  }
  
  free(top);
  printf("===============================\n\n");
}

void profiler_save_report(RuntimeProfiler *profiler, const char *filename) {
  if (!profiler || !filename) return;
  
  FILE *f = fopen(filename, "w");
  if (!f) return;
  
  fprintf(f, "# Nova Runtime Profile Report\n\n");
  fprintf(f, "## Summary\n");
  fprintf(f, "- Functions: %zu\n", profiler->entry_count);
  fprintf(f, "- Overhead: %.3f ms\n\n", profiler->profiling_overhead_ns / 1e6);
  
  fprintf(f, "## Top Functions\n\n");
  fprintf(f, "| Function | Calls | Total (ms) | Self (ms) | Avg (μs) |\n");
  fprintf(f, "|----------|-------|------------|-----------|----------|\n");
  
  ProfileEntry **top = profiler_get_top_functions(profiler, 50);
  
  for (size_t i = 0; i < 50 && i < profiler->entry_count; i++) {
    ProfileEntry *e = top[i];
    fprintf(f, "| %s | %llu | %.3f | %.3f | %.3f |\n",
            e->function_name, (unsigned long long)e->call_count,
            e->total_time_ns / 1e6, e->self_time_ns / 1e6,
            e->call_count > 0 ? (e->total_time_ns / (double)e->call_count) / 1000.0 : 0.0);
  }
  
  free(top);
  fclose(f);
}
