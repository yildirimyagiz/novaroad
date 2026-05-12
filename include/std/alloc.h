/**
 * @file alloc.h
 * @brief Memory allocation and custom allocators
 * 
 * Nova provides multiple memory allocation strategies:
 * - General allocator (malloc/free wrapper)
 * - Arena allocator (fast bump allocation, bulk free)
 * - Pool allocator (fixed-size objects)
 * - Aligned allocation for SIMD/DMA
 */

#ifndef NOVA_ALLOC_H
#define NOVA_ALLOC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * General Memory Allocation
 * ======================================================================== */

/**
 * @brief Allocate memory
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void *nova_alloc(size_t size);

/**
 * @brief Allocate zeroed memory
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to zeroed memory, or NULL on failure
 */
void *nova_calloc(size_t count, size_t size);

/**
 * @brief Reallocate memory
 * @param ptr Pointer to previously allocated memory
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *nova_realloc(void *ptr, size_t new_size);

/**
 * @brief Free memory
 * @param ptr Pointer to memory to free
 */
void nova_free(void *ptr);

/**
 * @brief Get size of allocated block
 * @param ptr Pointer to allocated memory
 * @return Size in bytes, or 0 if unknown
 */
size_t nova_alloc_size(void *ptr);

/**
 * @brief Duplicate memory block
 * @param ptr Source pointer
 * @param size Size to copy
 * @return Pointer to new allocation, or NULL on failure
 */
void *nova_memdup(const void *ptr, size_t size);

/* ========================================================================
 * Aligned Allocation (for SIMD, DMA, etc.)
 * ======================================================================== */

/**
 * @brief Allocate aligned memory
 * @param alignment Required alignment (must be power of 2)
 * @param size Number of bytes to allocate
 * @return Pointer to aligned memory, or NULL on failure
 */
void *nova_aligned_alloc(size_t alignment, size_t size);

/**
 * @brief Free aligned memory
 * @param ptr Pointer to aligned memory
 */
void nova_aligned_free(void *ptr);

/**
 * @brief Check if pointer is aligned
 * @param ptr Pointer to check
 * @param alignment Required alignment
 * @return true if aligned, false otherwise
 */
bool nova_is_aligned(const void *ptr, size_t alignment);

/* ========================================================================
 * Arena Allocator (Fast bump allocation, bulk free)
 * ======================================================================== */

typedef struct nova_arena nova_arena_t;

/**
 * @brief Create arena allocator
 * @param capacity Initial capacity in bytes (0 for default)
 * @return Arena handle, or NULL on failure
 */
nova_arena_t *nova_arena_create(size_t capacity);

/**
 * @brief Allocate from arena
 * @param arena Arena handle
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory (never NULL if arena valid)
 */
void *nova_arena_alloc(nova_arena_t *arena, size_t size);

/**
 * @brief Allocate aligned memory from arena
 * @param arena Arena handle
 * @param alignment Required alignment
 * @param size Number of bytes to allocate
 * @return Pointer to aligned memory
 */
void *nova_arena_alloc_aligned(nova_arena_t *arena, size_t alignment, size_t size);

/**
 * @brief Reset arena (keep capacity, free all allocations)
 * @param arena Arena handle
 */
void nova_arena_reset(nova_arena_t *arena);

/**
 * @brief Get arena memory usage
 * @param arena Arena handle
 * @return Bytes currently allocated
 */
size_t nova_arena_usage(nova_arena_t *arena);

/**
 * @brief Destroy arena and free all memory
 * @param arena Arena handle
 */
void nova_arena_destroy(nova_arena_t *arena);

/* ========================================================================
 * Pool Allocator (Fixed-size object pools)
 * ======================================================================== */

typedef struct nova_pool nova_pool_t;

/**
 * @brief Create object pool
 * @param object_size Size of each object
 * @param capacity Initial number of objects (0 for default)
 * @return Pool handle, or NULL on failure
 */
nova_pool_t *nova_pool_create(size_t object_size, size_t capacity);

/**
 * @brief Allocate object from pool
 * @param pool Pool handle
 * @return Pointer to object, or NULL if pool exhausted
 */
void *nova_pool_alloc(nova_pool_t *pool);

/**
 * @brief Return object to pool
 * @param pool Pool handle
 * @param ptr Object to free
 */
void nova_pool_free(nova_pool_t *pool, void *ptr);

/**
 * @brief Get pool statistics
 * @param pool Pool handle
 * @param total Output: total objects
 * @param available Output: available objects
 */
void nova_pool_stats(nova_pool_t *pool, size_t *total, size_t *available);

/**
 * @brief Destroy pool
 * @param pool Pool handle
 */
void nova_pool_destroy(nova_pool_t *pool);

/* ========================================================================
 * Memory Statistics
 * ======================================================================== */

typedef struct {
    size_t total_allocated;     /**< Total bytes allocated */
    size_t total_freed;          /**< Total bytes freed */
    size_t current_usage;        /**< Current memory usage */
    size_t peak_usage;           /**< Peak memory usage */
    size_t allocation_count;     /**< Number of allocations */
    size_t free_count;           /**< Number of frees */
} nova_alloc_stats_t;

/**
 * @brief Get global allocation statistics
 * @param stats Output statistics structure
 */
void nova_alloc_get_stats(nova_alloc_stats_t *stats);

/**
 * @brief Reset allocation statistics
 */
void nova_alloc_reset_stats(void);

/**
 * @brief Enable/disable allocation tracking
 * @param enabled true to enable tracking
 */
void nova_alloc_set_tracking(bool enabled);

/* ========================================================================
 * Memory Debugging
 * ======================================================================== */

#ifdef NOVA_ALLOC_DEBUG

/**
 * @brief Check for memory leaks
 * @return Number of leaks detected
 */
size_t nova_alloc_check_leaks(void);

/**
 * @brief Dump allocation information
 */
void nova_alloc_dump(void);

#endif /* NOVA_ALLOC_DEBUG */

/* ========================================================================
 * Convenience Macros
 * ======================================================================== */

/**
 * @brief Allocate and cast to type
 */
#define NOVA_NEW(type) ((type *)nova_alloc(sizeof(type)))

/**
 * @brief Allocate array of type
 */
#define NOVA_NEW_ARRAY(type, count) ((type *)nova_alloc(sizeof(type) * (count)))

/**
 * @brief Allocate zeroed type
 */
#define NOVA_NEW_ZERO(type) ((type *)nova_calloc(1, sizeof(type)))

/**
 * @brief Free and nullify pointer
 */
#define NOVA_FREE(ptr) do { nova_free(ptr); (ptr) = NULL; } while(0)

#ifdef __cplusplus
}
#endif

#endif /* NOVA_ALLOC_H */
