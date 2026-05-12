/**
 * @file nova_profiler.h
 * @brief Nova Performance Profiling System
 * 
 * Features:
 * - Function-level profiling
 * - Memory profiling
 * - GPU profiling
 * - Hotspot detection
 * - Performance visualization
 */

#ifndef NOVA_PROFILER_H
#define NOVA_PROFILER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Profiling Configuration
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_PROFILE_CPU,
    NOVA_PROFILE_GPU,
    NOVA_PROFILE_MEMORY,
    NOVA_PROFILE_ALL
} nova_profile_mode_t;

typedef struct {
    nova_profile_mode_t mode;
    bool enable_stack_trace;
    bool enable_memory_tracking;
    bool enable_gpu_events;
    size_t sampling_interval_us;  // Microseconds
    const char* output_file;
} nova_profiler_config_t;

/**
 * Initialize profiler
 */
void nova_profiler_init(nova_profiler_config_t config);

/**
 * Cleanup profiler
 */
void nova_profiler_cleanup(void);

/**
 * Start profiling session
 */
void nova_profiler_start(void);

/**
 * Stop profiling session
 */
void nova_profiler_stop(void);

// ═══════════════════════════════════════════════════════════════════════════
// Function Profiling
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    const char* function_name;
    const char* file;
    int line;
    uint64_t call_count;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double avg_time_ns;
} nova_function_profile_t;

/**
 * Profile function entry
 */
void nova_profile_function_enter(const char* name, const char* file, int line);

/**
 * Profile function exit
 */
void nova_profile_function_exit(const char* name);

/**
 * Macro for automatic function profiling
 */
#define NOVA_PROFILE_FUNCTION() \
    nova_profile_function_enter(__func__, __FILE__, __LINE__); \
    __attribute__((cleanup(nova_profile_cleanup_helper)))

/**
 * Get function profiling results
 */
nova_function_profile_t* nova_profiler_get_function_stats(size_t* count);

// ═══════════════════════════════════════════════════════════════════════════
// Memory Profiling
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t deallocation_count;
    size_t live_allocations;
} nova_memory_stats_t;

typedef struct {
    void* address;
    size_t size;
    const char* file;
    int line;
    uint64_t timestamp;
} nova_allocation_info_t;

/**
 * Track memory allocation
 */
void nova_profile_alloc(void* ptr, size_t size, const char* file, int line);

/**
 * Track memory deallocation
 */
void nova_profile_free(void* ptr);

/**
 * Get memory statistics
 */
nova_memory_stats_t nova_profiler_get_memory_stats(void);

/**
 * Get all active allocations
 */
nova_allocation_info_t* nova_profiler_get_allocations(size_t* count);

/**
 * Detect memory leaks
 */
size_t nova_profiler_detect_leaks(void);

// ═══════════════════════════════════════════════════════════════════════════
// GPU Profiling
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    const char* kernel_name;
    uint64_t launch_count;
    uint64_t total_time_ns;
    double avg_time_ns;
    size_t grid_size[3];
    size_t block_size[3];
} nova_gpu_kernel_profile_t;

typedef struct {
    size_t total_transfers;
    size_t bytes_h2d;  // Host to device
    size_t bytes_d2h;  // Device to host
    uint64_t transfer_time_ns;
    double bandwidth_gbps;
} nova_gpu_memory_stats_t;

/**
 * Profile GPU kernel launch
 */
void nova_profile_gpu_kernel_start(const char* name);

/**
 * Profile GPU kernel end
 */
void nova_profile_gpu_kernel_end(const char* name);

/**
 * Profile GPU memory transfer
 */
void nova_profile_gpu_transfer(size_t bytes, bool h2d);

/**
 * Get GPU kernel statistics
 */
nova_gpu_kernel_profile_t* nova_profiler_get_gpu_kernels(size_t* count);

/**
 * Get GPU memory statistics
 */
nova_gpu_memory_stats_t nova_profiler_get_gpu_memory_stats(void);

// ═══════════════════════════════════════════════════════════════════════════
// Hotspot Detection
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    const char* location;
    double cpu_time_percent;
    uint64_t execution_count;
    const char* recommendation;
} nova_hotspot_t;

/**
 * Detect performance hotspots
 */
nova_hotspot_t* nova_profiler_detect_hotspots(size_t* count);

// ═══════════════════════════════════════════════════════════════════════════
// Report Generation
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_REPORT_TEXT,
    NOVA_REPORT_JSON,
    NOVA_REPORT_HTML,
    NOVA_REPORT_FLAMEGRAPH
} nova_report_format_t;

/**
 * Generate profiling report
 */
void nova_profiler_generate_report(nova_report_format_t format, const char* output_file);

/**
 * Print profiling summary to console
 */
void nova_profiler_print_summary(void);

// ═══════════════════════════════════════════════════════════════════════════
// Performance Counters
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_misses;
    uint64_t branch_misses;
    double ipc;  // Instructions per cycle
    double cache_miss_rate;
} nova_perf_counters_t;

/**
 * Read performance counters
 */
nova_perf_counters_t nova_profiler_read_counters(void);

// ═══════════════════════════════════════════════════════════════════════════
// Tracing
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    const char* name;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    int thread_id;
    const char* category;
} nova_trace_event_t;

/**
 * Begin trace event
 */
void nova_trace_begin(const char* name, const char* category);

/**
 * End trace event
 */
void nova_trace_end(const char* name);

/**
 * Export trace in Chrome Trace Format
 */
void nova_profiler_export_trace(const char* output_file);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_PROFILER_H */
