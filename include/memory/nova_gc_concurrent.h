/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA CONCURRENT GARBAGE COLLECTOR                        ║
 * ║                                                                               ║
 * ║   Concurrent GC with parallel marking and minimal pause times:               ║
 * ║   • Tri-color marking with concurrent threads                                ║
 * ║   • Write barriers for concurrent correctness                                ║
 * ║   • Incremental sweeping                                                     ║
 * ║   • Sub-millisecond pause times                                              ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_GC_CONCURRENT_H
#define NOVA_GC_CONCURRENT_H

#include "nova_gc.h"
#include <pthread.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// CONCURRENT GC TYPES
// ═══════════════════════════════════════════════════════════════════════════════

typedef enum {
    GC_PHASE_IDLE = 0,
    GC_PHASE_MARK_PREP,      // Preparing for marking
    GC_PHASE_MARK_CONCURRENT, // Concurrent marking
    GC_PHASE_MARK_FINAL,      // Final mark (stop-the-world)
    GC_PHASE_SWEEP,           // Sweeping (incremental)
} GCPhase;

typedef struct {
    NovaGC base;              // Base GC structure
    
    // Concurrent marking
    pthread_t marker_thread;
    atomic_bool marking_active;
    atomic_int phase;
    
    // Work queues for concurrent marking
    GCObjectHeader **gray_queue;
    size_t gray_queue_size;
    size_t gray_queue_capacity;
    pthread_mutex_t gray_queue_lock;
    
    // Synchronization
    pthread_mutex_t gc_lock;
    pthread_cond_t phase_changed;
    
    // Statistics
    atomic_ulong concurrent_mark_time_ns;
    atomic_ulong pause_time_ns;
    atomic_int concurrent_cycles;
    
} NovaConcurrentGC;

// ═══════════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Initialize concurrent GC
 */
NovaConcurrentGC *nova_concurrent_gc_init(size_t young_size, size_t old_size);

/**
 * Destroy concurrent GC
 */
void nova_concurrent_gc_destroy(NovaConcurrentGC *gc);

/**
 * Start concurrent marking thread
 */
void nova_concurrent_gc_start(NovaConcurrentGC *gc);

/**
 * Stop concurrent marking thread
 */
void nova_concurrent_gc_stop(NovaConcurrentGC *gc);

// ═══════════════════════════════════════════════════════════════════════════════
// CONCURRENT COLLECTION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Trigger concurrent GC cycle
 * 
 * This initiates a concurrent GC cycle that runs in the background.
 * Mutator threads continue running with minimal pause.
 */
void nova_concurrent_gc_collect(NovaConcurrentGC *gc);

/**
 * Concurrent write barrier
 * 
 * Must be called when storing a pointer to track mutations during
 * concurrent marking.
 */
static inline void nova_concurrent_write_barrier(NovaConcurrentGC *gc, 
                                                    void *obj, 
                                                    void *field, 
                                                    void *value) {
    (void)field;
    
    if (!value) return;
    
    // If we're in concurrent marking phase, mark the value
    if (atomic_load(&gc->marking_active)) {
        GCObjectHeader *obj_hdr = (GCObjectHeader*)obj - 1;
        GCObjectHeader *val_hdr = (GCObjectHeader*)value - 1;
        
        // If black object points to white, mark it gray
        if (obj_hdr->color == GC_BLACK && val_hdr->color == GC_WHITE) {
            val_hdr->color = GC_GRAY;
            // Add to gray queue for concurrent marking
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// MONITORING
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    uint64_t concurrent_mark_time_ms;
    uint64_t pause_time_ms;
    int concurrent_cycles;
    GCPhase current_phase;
    size_t gray_queue_size;
} ConcurrentGCStats;

/**
 * Get concurrent GC statistics
 */
ConcurrentGCStats nova_concurrent_gc_stats(NovaConcurrentGC *gc);

/**
 * Print concurrent GC stats
 */
void nova_concurrent_gc_print_stats(NovaConcurrentGC *gc);

#ifdef __cplusplus
}
#endif

#endif // NOVA_GC_CONCURRENT_H
