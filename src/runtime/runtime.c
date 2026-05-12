/**
 * @file runtime.c
 * @brief Nova runtime system implementation
 */

#include "runtime/runtime.h"
#include "plugin/plugin.h"
#include "runtime/gc.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static struct {
    nova_gc_t *gc;
    bool initialized;
    time_t start_time;
    struct timespec start_time_mono;
    size_t total_allocations;
    size_t total_frees;
    size_t active_coroutines;
    size_t active_actors;
    nova_error_code_t last_error;

    // Performance tracking
    atomic_size_t function_calls;
    atomic_size_t exceptions_thrown;
    atomic_size_t exceptions_caught;
    atomic_size_t panics;
    uint64_t total_cpu_time_ns;
    uint64_t total_wall_time_ns;
    size_t context_switches;
    size_t syscalls;

    // Memory tracking
    atomic_size_t heap_peak;
    atomic_size_t current_heap_used;
    size_t gc_collections;
    size_t gc_freed_bytes;

    // Concurrency tracking
    atomic_size_t peak_coroutines;
    atomic_size_t peak_actors;
    atomic_size_t peak_threads;
    size_t total_coroutines_created;
    size_t total_actors_created;
    size_t total_threads_created;
} runtime;

static void *error_handler = NULL;
static void *panic_handler = NULL;
static void *init_hook = NULL;
static void *shutdown_hook = NULL;

// Mock interval callbacks
#define MAX_INTERVALS 64
static struct {
    void (*callback)(void);
    uint32_t interval_ms;
    uint32_t last_call;
    bool active;
} intervals[MAX_INTERVALS];
static size_t interval_count = 0;
static pthread_mutex_t interval_mutex = PTHREAD_MUTEX_INITIALIZER;

// Performance monitoring
static struct timespec last_stats_time;
static nova_runtime_stats_t last_stats;

uint64_t nova_get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

uint64_t nova_get_wall_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

static void update_performance_stats(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Simple CPU time estimation (in a real implementation, we'd use getrusage)
    uint64_t wall_time = (now.tv_sec - last_stats_time.tv_sec) * 1000000000ULL +
                         (now.tv_nsec - last_stats_time.tv_nsec);
    runtime.total_wall_time_ns += wall_time;

    // Estimate CPU time as 80% of wall time (rough approximation)
    runtime.total_cpu_time_ns += wall_time * 8 / 10;

    last_stats_time = now;
}

int nova_runtime_init(void)
{
    if (runtime.initialized) {
        return 0;
    }

    runtime.start_time = time(NULL);
    clock_gettime(CLOCK_MONOTONIC, &runtime.start_time_mono);

    runtime.gc = nova_gc_init(NOVA_GC_GENERATIONAL);
    if (!runtime.gc) {
        return -1;
    }

    // Initialize all tracking variables
    runtime.total_allocations = 0;
    runtime.total_frees = 0;
    runtime.active_coroutines = 0;
    runtime.active_actors = 0;
    runtime.last_error = NOVA_ERROR_NONE;

    atomic_init(&runtime.function_calls, 0);
    atomic_init(&runtime.exceptions_thrown, 0);
    atomic_init(&runtime.exceptions_caught, 0);
    atomic_init(&runtime.panics, 0);
    runtime.total_cpu_time_ns = 0;
    runtime.total_wall_time_ns = 0;
    runtime.context_switches = 0;
    runtime.syscalls = 0;

    atomic_init(&runtime.heap_peak, 0);
    atomic_init(&runtime.current_heap_used, 0);
    runtime.gc_collections = 0;
    runtime.gc_freed_bytes = 0;

    atomic_init(&runtime.peak_coroutines, 0);
    atomic_init(&runtime.peak_actors, 0);
    atomic_init(&runtime.peak_threads, 0);
    runtime.total_coroutines_created = 0;
    runtime.total_actors_created = 0;
    runtime.total_threads_created = 0;

    // Initialize stats timing
    clock_gettime(CLOCK_MONOTONIC, &last_stats_time);

    runtime.initialized = true;

    // Call init hook if set
    if (init_hook) {
        ((void (*)(void)) init_hook)();
    }

    return 0;
}

void nova_runtime_shutdown(void)
{
    if (!runtime.initialized) {
        return;
    }

    // Call shutdown hook if set
    if (shutdown_hook) {
        ((void (*)(void)) shutdown_hook)();
    }

    nova_gc_destroy(runtime.gc);
    runtime.initialized = false;
}

bool nova_runtime_is_initialized(void)
{
    return runtime.initialized;
}

const char *nova_runtime_version(void)
{
    return "0.1.0-production";
}

const char *nova_runtime_build_info(void)
{
    static char build_info[512];
    snprintf(build_info, sizeof(build_info),
             "Nova Runtime v%s\n"
             "Build: %s %s\n"
             "Compiler: %s %s\n"
             "Platform: %s\n"
             "Features: GC, Plugin System, Async Runtime",
             nova_runtime_version(), __DATE__, __TIME__, __VERSION__,
#if defined(__x86_64__)
             "x86_64",
#elif defined(__aarch64__)
             "ARM64",
#else
             "Unknown",
#endif
#if defined(__linux__)
             "Linux"
#elif defined(__APPLE__)
             "macOS"
#else
             "Unknown"
#endif
    );
    return build_info;
}

void nova_runtime_stats(nova_runtime_stats_t *stats)
{
    if (!stats)
        return;

    // Update performance stats
    update_performance_stats();

    nova_gc_stats_t gc_stats;
    nova_gc_stats(runtime.gc, &gc_stats);

    stats->heap_used =
        gc_stats.eden_used + gc_stats.survivor1_used + gc_stats.survivor2_used + gc_stats.old_used;
    stats->heap_allocated = gc_stats.total_allocated;
    stats->heap_peak = atomic_load(&runtime.heap_peak);
    stats->gc_collections = gc_stats.total_collections;
    stats->gc_freed = gc_stats.freed_objects;

    // Update peak heap usage
    if (stats->heap_used > stats->heap_peak) {
        atomic_store(&runtime.heap_peak, stats->heap_used);
    }

    // Concurrency statistics
    stats->active_coroutines = runtime.active_coroutines;
    stats->active_actors = runtime.active_actors;
    stats->active_threads = 1; // Main thread (simplified)
    stats->total_coroutines = runtime.total_coroutines_created;
    stats->total_actors = runtime.total_actors_created;

    // Update peaks
    if (stats->active_coroutines > atomic_load(&runtime.peak_coroutines)) {
        atomic_store(&runtime.peak_coroutines, stats->active_coroutines);
    }
    if (stats->active_actors > atomic_load(&runtime.peak_actors)) {
        atomic_store(&runtime.peak_actors, stats->active_actors);
    }
    if (stats->active_threads > atomic_load(&runtime.peak_threads)) {
        atomic_store(&runtime.peak_threads, stats->active_threads);
    }

    // Performance statistics
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    stats->uptime_ms = (now.tv_sec - runtime.start_time_mono.tv_sec) * 1000 +
                       (now.tv_nsec - runtime.start_time_mono.tv_nsec) / 1000000;

    stats->cpu_time_ms = runtime.total_cpu_time_ns / 1000000;
    stats->context_switches = runtime.context_switches;
    stats->syscalls = runtime.syscalls;

    // Error statistics
    stats->exceptions_thrown = atomic_load(&runtime.exceptions_thrown);
    stats->exceptions_caught = atomic_load(&runtime.exceptions_caught);
    stats->panics = atomic_load(&runtime.panics);

    // Store for delta calculations
    last_stats = *stats;
}

void nova_runtime_stats_print(void)
{
    nova_runtime_stats_t stats;
    nova_runtime_stats(&stats);

    printf("\n╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                           NOVA RUNTIME STATISTICS                           ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");

    printf("⏱️  TIMING\n");
    printf("   Uptime: %llu ms\n", (unsigned long long) stats.uptime_ms);
    printf("   CPU Time: %llu ms\n", (unsigned long long) stats.cpu_time_ms);
    printf("   Wall Time: %llu ms\n", (unsigned long long) stats.uptime_ms);

    printf("\n💾 MEMORY\n");
    printf("   Heap Used: %zu bytes\n", stats.heap_used);
    printf("   Heap Allocated: %zu bytes\n", stats.heap_allocated);
    printf("   Heap Peak: %zu bytes\n", stats.heap_peak);
    printf("   GC Collections: %zu\n", stats.gc_collections);
    printf("   GC Freed Objects: %zu\n", stats.gc_freed);

    printf("\n🔄 CONCURRENCY\n");
    printf("   Active Coroutines: %zu (peak: %zu)\n", stats.active_coroutines,
           atomic_load(&runtime.peak_coroutines));
    printf("   Active Actors: %zu (peak: %zu)\n", stats.active_actors,
           atomic_load(&runtime.peak_actors));
    printf("   Active Threads: %zu (peak: %zu)\n", stats.active_threads,
           atomic_load(&runtime.peak_threads));
    printf("   Total Coroutines Created: %zu\n", stats.total_coroutines);
    printf("   Total Actors Created: %zu\n", stats.total_actors);

    printf("\n⚡ PERFORMANCE\n");
    printf("   Function Calls: %zu\n", atomic_load(&runtime.function_calls));
    printf("   Context Switches: %zu\n", stats.context_switches);
    printf("   System Calls: %zu\n", stats.syscalls);

    printf("\n❌ ERROR HANDLING\n");
    printf("   Exceptions Thrown: %zu\n", stats.exceptions_thrown);
    printf("   Exceptions Caught: %zu\n", stats.exceptions_caught);
    printf("   Panics: %zu\n", stats.panics);

    printf("\n📊 EFFICIENCY\n");
    double gc_efficiency =
        stats.gc_collections > 0 ? (double) stats.gc_freed / (double) stats.gc_collections : 0.0;
    double memory_efficiency =
        stats.heap_allocated > 0 ? (double) stats.heap_used / (double) stats.heap_allocated : 0.0;

    printf("   GC Efficiency: %.2f objects/collection\n", gc_efficiency);
    printf("   Memory Efficiency: %.2f%%\n", memory_efficiency * 100.0);
    printf("   CPU Utilization: %.1f%%\n",
           stats.uptime_ms > 0 ? (double) stats.cpu_time_ms / (double) stats.uptime_ms * 100.0
                               : 0.0);

    printf("\n═══════════════════════════════════════════════════════════════════════════════\n\n");
}

void nova_runtime_stats_reset(void)
{
    runtime.total_allocations = 0;
    runtime.total_frees = 0;
    runtime.active_coroutines = 0;
    runtime.active_actors = 0;

    atomic_store(&runtime.function_calls, 0);
    atomic_store(&runtime.exceptions_thrown, 0);
    atomic_store(&runtime.exceptions_caught, 0);
    atomic_store(&runtime.panics, 0);
    runtime.total_cpu_time_ns = 0;
    runtime.total_wall_time_ns = 0;
    runtime.context_switches = 0;
    runtime.syscalls = 0;

    atomic_store(&runtime.heap_peak, 0);
    atomic_store(&runtime.current_heap_used, 0);
    runtime.gc_collections = 0;
    runtime.gc_freed_bytes = 0;

    atomic_store(&runtime.peak_coroutines, 0);
    atomic_store(&runtime.peak_actors, 0);
    atomic_store(&runtime.peak_threads, 0);
    runtime.total_coroutines_created = 0;
    runtime.total_actors_created = 0;
    runtime.total_threads_created = 0;

    runtime.start_time = time(NULL);
    clock_gettime(CLOCK_MONOTONIC, &runtime.start_time_mono);
    clock_gettime(CLOCK_MONOTONIC, &last_stats_time);
}

nova_runtime_config_t nova_runtime_config_default(void)
{
    nova_runtime_config_t config = {
        .heap_size = 128 * 1024 * 1024, // 128MB
        .stack_size = 8 * 1024 * 1024,  // 8MB
        .use_gc = true,
        .worker_threads = 0, // Auto-detect
        .max_coroutines = 100000,
        .max_actors = 10000,
        .enable_jit = true,
        .enable_profiling = true,
        .enable_tracing = false,
        .debug_mode = false,
        .strict_mode = true,
        .log_file = NULL,
        .log_level = 3, // Info
    };
    return config;
}

bool nova_runtime_config_validate(const nova_runtime_config_t *config)
{
    if (!config)
        return false;
    if (config->heap_size == 0 || config->heap_size > (size_t) 1 << 40)
        return false; // Max 1TB
    if (config->stack_size == 0 || config->stack_size > 1024 * 1024 * 1024)
        return false; // Max 1GB
    if (config->worker_threads > 1024)
        return false;
    if (config->max_coroutines == 0 || config->max_coroutines > 10000000)
        return false;
    if (config->max_actors == 0 || config->max_actors > 1000000)
        return false;
    if (config->log_level > 4)
        return false;
    return true;
}

int nova_runtime_init_with_config(const nova_runtime_config_t *config)
{
    if (!nova_runtime_config_validate(config)) {
        runtime.last_error = NOVA_ERROR_INVALID_ARGUMENT;
        return -1;
    }

    // Apply configuration (simplified - most options ignored in this implementation)
    (void) config; // Mark as used to avoid compiler warning

    return nova_runtime_init();
}

int nova_runtime_shutdown_timeout(uint32_t timeout_ms)
{
    (void) timeout_ms;
    nova_runtime_shutdown();
    return 0;
}

nova_error_code_t nova_runtime_get_error(void)
{
    return runtime.last_error;
}

const char *nova_runtime_error_string(nova_error_code_t error)
{
    switch (error) {
    case NOVA_ERROR_NONE:
        return "No error";
    case NOVA_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case NOVA_ERROR_STACK_OVERFLOW:
        return "Stack overflow";
    case NOVA_ERROR_DIVISION_BY_ZERO:
        return "Division by zero";
    case NOVA_ERROR_NULL_POINTER:
        return "Null pointer dereference";
    case NOVA_ERROR_ASSERTION_FAILED:
        return "Assertion failed";
    case NOVA_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case NOVA_ERROR_RESOURCE_EXHAUSTED:
        return "Resource exhausted";
    case NOVA_ERROR_TIMEOUT:
        return "Operation timed out";
    case NOVA_ERROR_PANIC:
        return "Runtime panic";
    default:
        return "Unknown error";
    }
}

void nova_runtime_clear_error(void)
{
    runtime.last_error = NOVA_ERROR_NONE;
}

void nova_runtime_set_error_handler(void (*handler)(nova_error_code_t error, const char *msg))
{
    error_handler = (void *) handler;
}

void nova_runtime_set_panic_handler(void (*handler)(const char *msg))
{
    panic_handler = (void *) handler;
}

void nova_runtime_panic(const char *message)
{
    fprintf(stderr, "\n💥 RUNTIME PANIC: %s\n", message);

    // Print stack trace (simplified)
    fprintf(stderr, "Stack trace:\n");
    // In a real implementation, we'd walk the stack

    if (panic_handler) {
        ((void (*)(const char *)) panic_handler)(message);
    }

    // Update panic count
    atomic_fetch_add(&runtime.panics, 1);

    abort();
}

void nova_runtime_on_init(void (*hook)(void))
{
    init_hook = (void *) hook;
}

void nova_runtime_on_shutdown(void (*hook)(void))
{
    shutdown_hook = (void *) hook;
}

int nova_runtime_set_interval(void (*callback)(void), uint32_t interval_ms)
{
    if (!callback || interval_count >= MAX_INTERVALS) {
        runtime.last_error = NOVA_ERROR_RESOURCE_EXHAUSTED;
        return -1;
    }

    pthread_mutex_lock(&interval_mutex);

    intervals[interval_count].callback = callback;
    intervals[interval_count].interval_ms = interval_ms;
    intervals[interval_count].last_call = 0;
    intervals[interval_count].active = true;

    int id = (int) interval_count;
    interval_count++;

    pthread_mutex_unlock(&interval_mutex);
    return id;
}

void nova_runtime_clear_interval(int callback_id)
{
    if (callback_id < 0 || (size_t) callback_id >= interval_count) {
        return;
    }

    pthread_mutex_lock(&interval_mutex);
    intervals[callback_id].active = false;
    pthread_mutex_unlock(&interval_mutex);
}

int nova_runtime_pause(void)
{
    // In a real implementation, we'd pause all threads
    return 0;
}

int nova_runtime_resume(void)
{
    // In a real implementation, we'd resume all threads
    return 0;
}

bool nova_runtime_is_paused(void)
{
    return false; // Simplified
}

void nova_runtime_gc(void)
{
    if (runtime.gc) {
        nova_gc_collect(runtime.gc);
        runtime.gc_collections++;
    }
}

void nova_runtime_yield(void)
{
    // Yield to scheduler
    sched_yield();
    runtime.context_switches++;
}

void nova_runtime_sleep(uint32_t ms)
{
    usleep(ms * 1000);
}

const char *nova_runtime_getenv(const char *name)
{
    runtime.syscalls++;
    return getenv(name);
}

int nova_runtime_setenv(const char *name, const char *value)
{
    runtime.syscalls++;
    return setenv(name, value, 1);
}

int nova_runtime_unsetenv(const char *name)
{
    runtime.syscalls++;
    return unsetenv(name);
}

int nova_runtime_set_limit(nova_limit_type_t type, size_t value)
{
    (void) type;
    (void) value;
    // In a real implementation, we'd set actual limits
    return 0;
}

size_t nova_runtime_get_limit(nova_limit_type_t type)
{
    (void) type;
    return SIZE_MAX; // No limit
}

nova_plugin_t *nova_runtime_load_plugin(const char *path)
{
    return nova_plugin_load(path);
}

void nova_runtime_unload_plugin(nova_plugin_t *plugin)
{
    nova_plugin_unload(plugin);
}

void *nova_runtime_plugin_symbol(nova_plugin_t *plugin, const char *symbol)
{
    return nova_plugin_symbol(plugin, symbol);
}
