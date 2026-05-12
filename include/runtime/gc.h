/**
 * @file gc.h
 * @brief Garbage collector interface
 *
 * Nova provides multiple GC algorithms:
 * - Mark-Sweep: Simple, stop-the-world collector
 * - Generational: Young/old generation collector
 * - Incremental: Low-latency incremental marking
 * - Concurrent: Parallel marking (future)
 *
 * Features:
 * - Automatic memory management
 * - Root set registration
 * - Weak references
 * - Finalizers
 * - GC statistics and tuning
 */

#ifndef NOVA_GC_H
#define NOVA_GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * GC Types and Configuration
 * ======================================================================== */

typedef struct nova_gc nova_gc_t;

/**
 * GC algorithm types
 */
typedef enum {
    NOVA_GC_MARK_SWEEP,   /**< Mark-and-sweep (simple, STW) */
    NOVA_GC_GENERATIONAL, /**< Generational (young/old) */
    NOVA_GC_INCREMENTAL,  /**< Incremental marking */
    NOVA_GC_CONCURRENT,   /**< Concurrent marking (future) */
} nova_gc_type_t;

/**
 * GC configuration
 */
typedef struct {
    nova_gc_type_t type;  /**< GC algorithm */
    size_t heap_size;     /**< Initial heap size (0 for default) */
    size_t min_heap_size; /**< Minimum heap size */
    size_t max_heap_size; /**< Maximum heap size (0 for unlimited) */

    /* Generational GC settings */
    size_t young_size;    /**< Young generation size */
    size_t old_threshold; /**< Promotion threshold (survivor count) */

    /* Incremental GC settings */
    size_t increment_size; /**< Bytes to mark per increment */

    /* Tuning parameters */
    double gc_pause_target;    /**< Target pause time (ms) */
    double heap_growth_factor; /**< Heap growth factor (1.5 = 150%) */
    bool aggressive_gc;        /**< Run GC more frequently */
} nova_gc_config_t;

/**
 * @brief Get default GC configuration
 * @param type GC algorithm type
 * @return Default configuration for the specified type
 */
nova_gc_config_t nova_gc_config_default(nova_gc_type_t type);

/* ========================================================================
 * GC Initialization
 * ======================================================================== */

/**
 * @brief Initialize garbage collector
 * @param type GC algorithm type
 * @return GC handle, or NULL on failure
 */
nova_gc_t *nova_gc_init(nova_gc_type_t type);

/**
 * @brief Initialize GC with custom configuration
 * @param config GC configuration
 * @return GC handle, or NULL on failure
 */
nova_gc_t *nova_gc_init_with_config(const nova_gc_config_t *config);

/**
 * @brief Destroy garbage collector
 * @param gc GC handle
 */
void nova_gc_destroy(nova_gc_t *gc);

/* ========================================================================
 * Memory Allocation
 * ======================================================================== */

/**
 * @brief Allocate GC-managed memory
 * @param gc GC handle
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void *nova_gc_alloc(nova_gc_t *gc, size_t size);

/**
 * @brief Allocate zeroed GC-managed memory
 * @param gc GC handle
 * @param size Number of bytes to allocate
 * @return Pointer to zeroed memory, or NULL on failure
 */
void *nova_gc_alloc_zeroed(nova_gc_t *gc, size_t size);

/**
 * @brief Allocate GC-managed memory with finalizer
 * @param gc GC handle
 * @param size Number of bytes to allocate
 * @param finalizer Finalizer function (called before deallocation)
 * @return Pointer to allocated memory, or NULL on failure
 */
void *nova_gc_alloc_with_finalizer(nova_gc_t *gc, size_t size, void (*finalizer)(void *));

/**
 * @brief Resize GC-managed allocation
 * @param gc GC handle
 * @param ptr Existing allocation
 * @param new_size New size in bytes
 * @return Pointer to resized memory, or NULL on failure
 */
void *nova_gc_realloc(nova_gc_t *gc, void *ptr, size_t new_size);

/* ========================================================================
 * Garbage Collection Control
 * ======================================================================== */

/**
 * @brief Trigger garbage collection
 * @param gc GC handle
 *
 * Performs a full GC cycle.
 */
void nova_gc_collect(nova_gc_t *gc);

/**
 * @brief Request minor collection (young generation only)
 * @param gc GC handle
 */
void nova_gc_collect_minor(nova_gc_t *gc);

/**
 * @brief Request major collection (full heap)
 * @param gc GC handle
 */
void nova_gc_collect_major(nova_gc_t *gc);

/**
 * @brief Enable/disable automatic GC
 * @param gc GC handle
 * @param enabled true to enable automatic GC
 */
void nova_gc_set_auto(nova_gc_t *gc, bool enabled);

/**
 * @brief Check if GC is enabled
 * @param gc GC handle
 * @return true if automatic GC is enabled
 */
bool nova_gc_is_auto(nova_gc_t *gc);

/**
 * @brief Pause garbage collection
 * @param gc GC handle
 *
 * Prevents GC from running until resumed.
 */
void nova_gc_pause(nova_gc_t *gc);

/**
 * @brief Resume garbage collection
 * @param gc GC handle
 */
void nova_gc_resume(nova_gc_t *gc);

/* ========================================================================
 * Root Set Management
 * ======================================================================== */

/**
 * @brief Add GC root
 * @param gc GC handle
 * @param ptr Pointer to root
 *
 * Roots are starting points for reachability analysis.
 */
void nova_gc_add_root(nova_gc_t *gc, void *ptr);

/**
 * @brief Remove GC root
 * @param gc GC handle
 * @param ptr Pointer to root
 */
void nova_gc_remove_root(nova_gc_t *gc, void *ptr);

/**
 * @brief Add range of roots
 * @param gc GC handle
 * @param start Start of memory range
 * @param end End of memory range
 */
void nova_gc_add_root_range(nova_gc_t *gc, void *start, void *end);

/**
 * @brief Clear all roots
 * @param gc GC handle
 */
void nova_gc_clear_roots(nova_gc_t *gc);

/* ========================================================================
 * Weak References
 * ======================================================================== */

typedef struct nova_weak_ref nova_weak_ref_t;

/**
 * @brief Create weak reference
 * @param gc GC handle
 * @param ptr Pointer to track weakly
 * @return Weak reference handle, or NULL on failure
 */
nova_weak_ref_t *nova_gc_weak_ref(nova_gc_t *gc, void *ptr);

/**
 * @brief Get pointer from weak reference
 * @param weak Weak reference handle
 * @return Pointer if still alive, NULL if collected
 */
void *nova_gc_weak_deref(nova_weak_ref_t *weak);

/**
 * @brief Destroy weak reference
 * @param weak Weak reference handle
 */
void nova_gc_weak_destroy(nova_weak_ref_t *weak);

/* ========================================================================
 * Write Barriers (for incremental/concurrent GC)
 * ======================================================================== */

/**
 * @brief Write barrier (update pointer field)
 * @param gc GC handle
 * @param obj Object being modified
 * @param field Pointer to field being updated
 * @param value New value
 *
 * Call this when updating a pointer field in a GC-managed object.
 */
void nova_gc_write_barrier(nova_gc_t *gc, void *obj, void **field, void *value);

/**
 * @brief Array write barrier
 * @param gc GC handle
 * @param array Array being modified
 * @param index Array index
 * @param value New value
 */
void nova_gc_array_write_barrier(nova_gc_t *gc, void *array, size_t index, void *value);

/* ========================================================================
 * GC Statistics
 * ======================================================================== */

typedef struct {
    /* Memory statistics */
    size_t heap_size;       /**< Current heap size */
    size_t heap_used;       /**< Bytes currently allocated */
    size_t heap_free;       /**< Bytes available */
    size_t heap_peak;       /**< Peak heap usage */
    size_t total_allocated; /**< Compatibility: Total bytes allocated */

    /* Collection statistics */
    size_t collections_total; /**< Total collections */
    size_t collections_minor; /**< Minor collections */
    size_t collections_major; /**< Major collections */
    size_t bytes_collected;   /**< Total bytes freed */
    size_t total_collections; /**< Compatibility: Total collections */

    /* Generational statistics */
    size_t young_size;     /**< Young generation size */
    size_t old_size;       /**< Old generation size */
    size_t promotions;     /**< Objects promoted to old gen */
    size_t eden_used;      /**< Compatibility: Eden space usage */
    size_t survivor1_used; /**< Compatibility: Survivor 1 usage */
    size_t survivor2_used; /**< Compatibility: Survivor 2 usage */
    size_t old_used;       /**< Compatibility: Old generation usage */

    /* Performance statistics */
    uint64_t gc_time_total_ms; /**< Total GC time */
    uint64_t gc_time_avg_ms;   /**< Average GC pause time */
    uint64_t gc_time_max_ms;   /**< Maximum GC pause time */
    double gc_overhead;        /**< GC time as % of total time */

    /* Object statistics */
    size_t objects_allocated; /**< Total objects allocated */
    size_t objects_live;      /**< Currently live objects */
    size_t objects_freed;     /**< Total objects freed */
    size_t freed_objects;     /**< Compatibility: Total objects freed */
    size_t objects_marked;    /**< Compatibility: Total objects marked */
    size_t collections;       /**< Compatibility: Total collections */
    size_t promoted_objects;  /**< Compatibility: Total objects promoted */
    size_t young_collections; /**< Compatibility: Young generation collections */
    size_t old_collections;   /**< Compatibility: Old generation collections */
    size_t young_gen_used;    /**< Compatibility: Young generation usage */
    size_t old_gen_used;      /**< Compatibility: Old generation usage */
    size_t live_objects;      /**< Compatibility: Live objects count */
    size_t total_freed;       /**< Compatibility: Total bytes freed */
} nova_gc_stats_t;

/**
 * @brief Get GC statistics
 * @param gc GC handle
 * @param stats Output: statistics structure
 */
void nova_gc_stats(nova_gc_t *gc, nova_gc_stats_t *stats);

/**
 * @brief Print GC statistics to stdout
 * @param gc GC handle
 */
void nova_gc_stats_print(nova_gc_t *gc);

/**
 * @brief Reset GC statistics
 * @param gc GC handle
 */
void nova_gc_stats_reset(nova_gc_t *gc);

/* ========================================================================
 * GC Tuning
 * ======================================================================== */

/**
 * @brief Set heap size limits
 * @param gc GC handle
 * @param min_size Minimum heap size
 * @param max_size Maximum heap size (0 for unlimited)
 */
void nova_gc_set_heap_limits(nova_gc_t *gc, size_t min_size, size_t max_size);

/**
 * @brief Set GC pause target
 * @param gc GC handle
 * @param target_ms Target pause time in milliseconds
 */
void nova_gc_set_pause_target(nova_gc_t *gc, double target_ms);

/**
 * @brief Set heap growth factor
 * @param gc GC handle
 * @param factor Growth factor (e.g., 1.5 for 150%)
 */
void nova_gc_set_growth_factor(nova_gc_t *gc, double factor);

/**
 * @brief Set collection threshold
 * @param gc GC handle
 * @param threshold Trigger GC when heap usage exceeds this percentage
 */
void nova_gc_set_threshold(nova_gc_t *gc, double threshold);

/* ========================================================================
 * Debugging and Profiling
 * ======================================================================== */

/**
 * @brief Enable GC logging
 * @param gc GC handle
 * @param enabled true to enable logging
 */
void nova_gc_set_logging(nova_gc_t *gc, bool enabled);

/**
 * @brief Dump heap contents (for debugging)
 * @param gc GC handle
 */
void nova_gc_dump_heap(nova_gc_t *gc);

/**
 * @brief Verify heap integrity
 * @param gc GC handle
 * @return true if heap is valid, false if corrupted
 */
bool nova_gc_verify_heap(nova_gc_t *gc);

/**
 * @brief Get object size
 * @param gc GC handle
 * @param ptr Pointer to GC-managed object
 * @return Size of object in bytes
 */
size_t nova_gc_object_size(nova_gc_t *gc, void *ptr);

/**
 * @brief Check if pointer is GC-managed
 * @param gc GC handle
 * @param ptr Pointer to check
 * @return true if managed by this GC
 */
bool nova_gc_is_managed(nova_gc_t *gc, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_GC_H */
