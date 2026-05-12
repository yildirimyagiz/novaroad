/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA GARBAGE COLLECTOR (ZGC)                             ║
 * ║                                                                               ║
 * ║   Advanced Tri-Color Marking & Generational Garbage Collector                ║
 * ║   • Concurrent marking with low pause times                                  ║
 * ║   • Generational collection (Young/Old generations)                          ║
 * ║   • Write barriers for incremental collection                                ║
 * ║   • SIMD-optimized mark-sweep                                                ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_GC_H
#define NOVA_GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// GC OBJECT HEADER
// ═══════════════════════════════════════════════════════════════════════════════

typedef enum {
    GC_WHITE = 0,  // Not visited (candidate for collection)
    GC_GRAY  = 1,  // Visited but children not scanned
    GC_BLACK = 2,  // Visited and children scanned (alive)
} GCColor;

typedef enum {
    GC_GEN_YOUNG = 0,  // Young generation (nursery)
    GC_GEN_OLD   = 1,  // Old generation (tenured)
} GCGeneration;

typedef struct GCObjectHeader {
    GCColor color;
    GCGeneration generation;
    uint32_t age;              // Survival count for promotion
    uint32_t size;             // Object size in bytes
    bool marked;               // Mark bit for collection
    bool pinned;               // Prevent moving/collection
    struct GCObjectHeader *next;  // Free list / gray list
    void (*finalizer)(void*);  // Optional destructor
} GCObjectHeader;

// ═══════════════════════════════════════════════════════════════════════════════
// GC HEAP STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    void *start;
    void *end;
    void *cursor;              // Bump allocator cursor
    size_t size;
    size_t used;
    GCObjectHeader *free_list;
} GCHeapRegion;

typedef struct {
    GCHeapRegion young_gen;    // Fast allocation arena
    GCHeapRegion old_gen;      // Long-lived objects
    
    GCObjectHeader *gray_list; // Objects to scan
    GCObjectHeader *all_objects; // All allocated objects
    
    size_t total_allocated;
    size_t total_freed;
    size_t collections;
    size_t young_collections;
    size_t old_collections;
    
    // GC tuning parameters
    size_t young_gen_limit;    // Trigger young GC at this size
    size_t heap_limit;         // Trigger full GC at this size
    uint32_t promotion_age;    // Age to promote to old gen
    
    bool gc_enabled;
    bool concurrent_mark;
} NovaGC;

// ═══════════════════════════════════════════════════════════════════════════════
// GC INITIALIZATION & CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Initialize the Nova Garbage Collector
 * @param young_size Size of young generation in bytes
 * @param old_size Size of old generation in bytes
 * @return Pointer to GC instance
 */
NovaGC *nova_gc_init(size_t young_size, size_t old_size);

/**
 * Destroy the GC and free all memory
 */
void nova_gc_destroy(NovaGC *gc);

/**
 * Enable/disable garbage collection
 */
void nova_gc_enable(NovaGC *gc, bool enabled);

/**
 * Set GC tuning parameters
 */
void nova_gc_set_young_limit(NovaGC *gc, size_t limit);
void nova_gc_set_heap_limit(NovaGC *gc, size_t limit);
void nova_gc_set_promotion_age(NovaGC *gc, uint32_t age);

// ═══════════════════════════════════════════════════════════════════════════════
// ALLOCATION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Allocate memory from the GC heap
 * @param gc GC instance
 * @param size Size in bytes
 * @return Pointer to allocated memory (NULL on failure)
 */
void *nova_gc_alloc(NovaGC *gc, size_t size);

/**
 * Allocate memory with finalizer
 * @param gc GC instance
 * @param size Size in bytes
 * @param finalizer Destructor function (called before freeing)
 * @return Pointer to allocated memory
 */
void *nova_gc_alloc_with_finalizer(NovaGC *gc, size_t size, 
                                      void (*finalizer)(void*));

/**
 * Pin an object (prevent GC from collecting it)
 */
void nova_gc_pin(void *ptr);

/**
 * Unpin an object
 */
void nova_gc_unpin(void *ptr);

// ═══════════════════════════════════════════════════════════════════════════════
// GARBAGE COLLECTION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Perform young generation collection (minor GC)
 * Fast collection of recently allocated objects
 */
void nova_gc_collect_young(NovaGC *gc);

/**
 * Perform full heap collection (major GC)
 * Collects both young and old generations
 */
void nova_gc_collect_full(NovaGC *gc);

/**
 * Automatic collection - runs appropriate GC based on heap usage
 */
void nova_gc_collect_auto(NovaGC *gc);

/**
 * Register a root object (stack variable, global, etc.)
 * Roots are always considered reachable
 */
void nova_gc_add_root(NovaGC *gc, void *root);

/**
 * Unregister a root object
 */
void nova_gc_remove_root(NovaGC *gc, void *root);

// ═══════════════════════════════════════════════════════════════════════════════
// WRITE BARRIERS (for incremental/concurrent GC)
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Write barrier - call when storing a pointer
 * Ensures GC correctness during incremental collection
 */
static inline void nova_gc_write_barrier(void *obj, void *field, void *value) {
    // If object is black and value is white, mark value gray
    GCObjectHeader *obj_hdr = (GCObjectHeader*)obj - 1;
    if (value) {
        GCObjectHeader *val_hdr = (GCObjectHeader*)value - 1;
        if (obj_hdr->color == GC_BLACK && val_hdr->color == GC_WHITE) {
            val_hdr->color = GC_GRAY;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// STATISTICS & DEBUGGING
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    size_t young_gen_used;
    size_t old_gen_used;
    size_t total_allocated;
    size_t total_freed;
    size_t live_objects;
    size_t collections;
    size_t young_collections;
    size_t old_collections;
    double last_gc_time_ms;
} GCStats;

/**
 * Get GC statistics
 */
GCStats nova_gc_get_stats(NovaGC *gc);

/**
 * Print GC statistics to stdout
 */
void nova_gc_print_stats(NovaGC *gc);

/**
 * Validate heap integrity (debug builds)
 */
bool nova_gc_validate_heap(NovaGC *gc);

// ═══════════════════════════════════════════════════════════════════════════════
// CONVENIENCE MACROS
// ═══════════════════════════════════════════════════════════════════════════════

// Allocate a typed object
#define GC_NEW(gc, type) \
    ((type*)nova_gc_alloc(gc, sizeof(type)))

// Allocate an array
#define GC_NEW_ARRAY(gc, type, count) \
    ((type*)nova_gc_alloc(gc, sizeof(type) * (count)))

// Write barrier for pointer assignment
#define GC_WRITE(obj, field, value) \
    do { \
        nova_gc_write_barrier(obj, &(obj)->field, value); \
        (obj)->field = (value); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // NOVA_GC_H
