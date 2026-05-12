/**
 * @file runtime.h
 * @brief Nova runtime system initialization and configuration
 *
 * The Nova runtime provides:
 * - Automatic initialization and cleanup
 * - Configuration management
 * - Resource limits and monitoring
 * - Performance statistics
 * - Error handling and recovery
 * - Plugin/extension system
 */

#ifndef NOVA_RUNTIME_H
#define NOVA_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Runtime Configuration
 * ======================================================================== */

typedef struct {
    /* Memory settings */
    size_t heap_size;  /**< Initial heap size (0 for default) */
    size_t stack_size; /**< Default stack size per coroutine */
    bool use_gc;       /**< Enable garbage collection */

    /* Concurrency settings */
    size_t worker_threads; /**< Number of worker threads (0 for auto) */
    size_t max_coroutines; /**< Max concurrent coroutines */
    size_t max_actors;     /**< Max concurrent actors */

    /* Performance settings */
    bool enable_jit;       /**< Enable JIT compilation */
    bool enable_profiling; /**< Enable performance profiling */
    bool enable_tracing;   /**< Enable execution tracing */

    /* Debug settings */
    bool debug_mode;      /**< Enable debug features */
    bool strict_mode;     /**< Enable strict error checking */
    const char *log_file; /**< Log file path (NULL for stderr) */
    int log_level;        /**< Log level (0=off, 1=error, 2=warn, 3=info, 4=debug) */
} nova_runtime_config_t;

/**
 * @brief Get default runtime configuration
 * @return Default configuration structure
 */
nova_runtime_config_t nova_runtime_config_default(void);

/**
 * @brief Validate runtime configuration
 * @param config Configuration to validate
 * @return true if valid, false otherwise
 */
bool nova_runtime_config_validate(const nova_runtime_config_t *config);

/* ========================================================================
 * Runtime Initialization
 * ======================================================================== */

/**
 * @brief Initialize Nova runtime with default configuration
 * @return 0 on success, negative error code on failure
 */
int nova_runtime_init(void);

/**
 * @brief Initialize Nova runtime with custom configuration
 * @param config Runtime configuration
 * @return 0 on success, negative error code on failure
 */
int nova_runtime_init_with_config(const nova_runtime_config_t *config);

/**
 * @brief Check if runtime is initialized
 * @return true if initialized
 */
bool nova_runtime_is_initialized(void);

/**
 * @brief Get runtime version
 * @return Version string
 */
const char *nova_runtime_version(void);

/**
 * @brief Get runtime build information
 * @return Build info string (compiler, date, flags, etc.)
 */
const char *nova_runtime_build_info(void);

/* ========================================================================
 * Runtime Shutdown
 * ======================================================================== */

/**
 * @brief Shutdown Nova runtime gracefully
 *
 * Waits for running tasks to complete, then releases all resources.
 */
void nova_runtime_shutdown(void);

/**
 * @brief Force shutdown Nova runtime
 *
 * Immediately terminates all tasks and releases resources.
 */
void nova_runtime_shutdown_force(void);

/**
 * @brief Shutdown with timeout
 * @param timeout_ms Maximum time to wait for graceful shutdown
 * @return 0 if shutdown completed, -1 if timed out
 */
int nova_runtime_shutdown_timeout(uint32_t timeout_ms);

/* ========================================================================
 * Runtime Statistics
 * ======================================================================== */

typedef struct {
    /* Memory statistics */
    size_t heap_used;      /**< Current heap usage in bytes */
    size_t heap_allocated; /**< Total heap allocated */
    size_t heap_peak;      /**< Peak heap usage */
    size_t gc_collections; /**< Number of GC runs */
    size_t gc_freed;       /**< Total bytes freed by GC */

    /* Concurrency statistics */
    size_t active_coroutines; /**< Currently active coroutines */
    size_t active_actors;     /**< Currently active actors */
    size_t active_threads;    /**< Currently active threads */
    size_t total_coroutines;  /**< Total coroutines created */
    size_t total_actors;      /**< Total actors created */

    /* Performance statistics */
    uint64_t uptime_ms;      /**< Runtime uptime in milliseconds */
    uint64_t cpu_time_ms;    /**< Total CPU time used */
    size_t context_switches; /**< Number of context switches */
    size_t syscalls;         /**< Number of system calls */

    /* Error statistics */
    size_t exceptions_thrown; /**< Total exceptions thrown */
    size_t exceptions_caught; /**< Total exceptions caught */
    size_t panics;            /**< Number of panics */
} nova_runtime_stats_t;

/**
 * @brief Get runtime statistics
 * @param stats Output: statistics structure
 */
void nova_runtime_stats(nova_runtime_stats_t *stats);

/**
 * @brief Print runtime statistics to stdout
 */
void nova_runtime_stats_print(void);

/**
 * @brief Reset runtime statistics
 */
void nova_runtime_stats_reset(void);

/* ========================================================================
 * Resource Limits
 * ======================================================================== */

typedef enum {
    NOVA_LIMIT_HEAP_SIZE,        /**< Maximum heap size */
    NOVA_LIMIT_STACK_SIZE,       /**< Maximum stack size */
    NOVA_LIMIT_COROUTINES,       /**< Maximum coroutines */
    NOVA_LIMIT_ACTORS,           /**< Maximum actors */
    NOVA_LIMIT_THREADS,          /**< Maximum threads */
    NOVA_LIMIT_FILE_DESCRIPTORS, /**< Maximum open file descriptors */
    NOVA_LIMIT_CPU_TIME,         /**< Maximum CPU time (ms) */
    NOVA_LIMIT_WALL_TIME,        /**< Maximum wall time (ms) */
} nova_limit_type_t;

/**
 * @brief Set resource limit
 * @param type Limit type
 * @param value Limit value (0 for unlimited)
 * @return 0 on success, -1 on failure
 */
int nova_runtime_set_limit(nova_limit_type_t type, size_t value);

/**
 * @brief Get resource limit
 * @param type Limit type
 * @return Current limit value
 */
size_t nova_runtime_get_limit(nova_limit_type_t type);

/* ========================================================================
 * Error Handling
 * ======================================================================== */

typedef enum {
    NOVA_ERROR_NONE = 0,
    NOVA_ERROR_OUT_OF_MEMORY,
    NOVA_ERROR_STACK_OVERFLOW,
    NOVA_ERROR_DIVISION_BY_ZERO,
    NOVA_ERROR_NULL_POINTER,
    NOVA_ERROR_ASSERTION_FAILED,
    NOVA_ERROR_INVALID_ARGUMENT,
    NOVA_ERROR_RESOURCE_EXHAUSTED,
    NOVA_ERROR_TIMEOUT,
    NOVA_ERROR_IO,
    NOVA_ERROR_PANIC,
    NOVA_ERROR_UNKNOWN,
} nova_error_code_t;

/**
 * @brief Get last runtime error
 * @return Error code
 */
nova_error_code_t nova_runtime_get_error(void);

/**
 * @brief Get error message
 * @param error Error code
 * @return Human-readable error message
 */
const char *nova_runtime_error_string(nova_error_code_t error);

/**
 * @brief Clear last error
 */
void nova_runtime_clear_error(void);

/**
 * @brief Set error handler callback
 * @param handler Error handler function
 */
void nova_runtime_set_error_handler(void (*handler)(nova_error_code_t error, const char *msg));

/* ========================================================================
 * Panic Handler
 * ======================================================================== */

/**
 * @brief Trigger runtime panic
 * @param message Panic message
 *
 * This will:
 * 1. Print panic message and backtrace
 * 2. Call panic handler if set
 * 3. Abort the program
 */
void nova_runtime_panic(const char *message) __attribute__((noreturn));

/**
 * @brief Set panic handler
 * @param handler Panic handler function
 */
void nova_runtime_set_panic_handler(void (*handler)(const char *msg));

/* ========================================================================
 * Hooks and Callbacks
 * ======================================================================== */

/**
 * @brief Register initialization hook
 * @param hook Function to call after runtime init
 */
void nova_runtime_on_init(void (*hook)(void));

/**
 * @brief Register shutdown hook
 * @param hook Function to call before runtime shutdown
 */
void nova_runtime_on_shutdown(void (*hook)(void));

/**
 * @brief Register periodic callback
 * @param callback Function to call periodically
 * @param interval_ms Interval in milliseconds
 * @return Callback ID (for cancellation)
 */
int nova_runtime_set_interval(void (*callback)(void), uint32_t interval_ms);

/**
 * @brief Cancel periodic callback
 * @param callback_id Callback ID from set_interval
 */
void nova_runtime_clear_interval(int callback_id);

/* ========================================================================
 * Timing
 * ======================================================================== */

/**
 * @brief Get monotonic time in nanoseconds
 * @return Time in nanoseconds
 */
uint64_t nova_get_time_ns(void);

/**
 * @brief Get wall time in nanoseconds
 * @return Time in nanoseconds
 */
uint64_t nova_get_wall_time_ns(void);

/* ========================================================================
 * Runtime Control
 * ======================================================================== */

/**
 * @brief Pause runtime execution
 * @return 0 on success, -1 on failure
 */
int nova_runtime_pause(void);

/**
 * @brief Resume runtime execution
 * @return 0 on success, -1 on failure
 */
int nova_runtime_resume(void);

/**
 * @brief Check if runtime is paused
 * @return true if paused
 */
bool nova_runtime_is_paused(void);

/**
 * @brief Request garbage collection
 */
void nova_runtime_gc(void);

/**
 * @brief Yield to scheduler
 */
void nova_runtime_yield(void);

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void nova_runtime_sleep(uint32_t ms);

/* ========================================================================
 * Environment Variables
 * ======================================================================== */

/**
 * @brief Get environment variable
 * @param name Variable name
 * @return Variable value, or NULL if not found
 */
const char *nova_runtime_getenv(const char *name);

/**
 * @brief Set environment variable
 * @param name Variable name
 * @param value Variable value
 * @return 0 on success, -1 on failure
 */
int nova_runtime_setenv(const char *name, const char *value);

/**
 * @brief Unset environment variable
 * @param name Variable name
 * @return 0 on success, -1 on failure
 */
int nova_runtime_unsetenv(const char *name);

/* ========================================================================
 * Plugin System
 * ======================================================================== */

typedef struct nova_plugin nova_plugin_t;

/**
 * @brief Load plugin from file
 * @param path Plugin file path (.so, .dylib, .dll)
 * @return Plugin handle, or NULL on failure
 */
nova_plugin_t *nova_runtime_load_plugin(const char *path);

/**
 * @brief Unload plugin
 * @param plugin Plugin handle
 */
void nova_runtime_unload_plugin(nova_plugin_t *plugin);

/**
 * @brief Get plugin symbol
 * @param plugin Plugin handle
 * @param symbol Symbol name
 * @return Symbol pointer, or NULL if not found
 */
void *nova_runtime_plugin_symbol(nova_plugin_t *plugin, const char *symbol);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_RUNTIME_H */
