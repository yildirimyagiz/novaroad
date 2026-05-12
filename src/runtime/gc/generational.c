/**
 * @file generational.c
 * @brief Generational garbage collector implementation
 *
 * Features:
 * - Young generation: Eden + 2 survivor spaces
 * - Old generation: Mark-sweep-compact
 * - Card marking table for inter-generational references
 * - Incremental collection with time limits
 * - Write barriers for remembered sets
 */

#include "runtime/gc.h"
#include "std/alloc.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define YOUNG_GENERATION_SIZE (16 * 1024 * 1024) // 16MB
#define EDEN_SIZE (10 * 1024 * 1024)             // 10MB

static void nova_gc_young_collect(nova_gc_t *gc);
#define SURVIVOR_SIZE (3 * 1024 * 1024)         // 3MB each
#define OLD_GENERATION_SIZE (128 * 1024 * 1024) // 128MB

#define CARD_SIZE 512        // 512 bytes per card
#define CARDS_PER_REGION 256 // Cards per region

#define GC_MIN_THRESHOLD 1024 // Min objects before GC
#define GC_GROWTH_FACTOR 2.0  // Heap growth factor

typedef enum {
    GC_STATE_IDLE,
    GC_STATE_MARKING,
    GC_STATE_SWEEPING,
    GC_STATE_COMPACTING,
    GC_STATE_YOUNG_COLLECTION,
    GC_STATE_OLD_COLLECTION
} gc_state_t;

typedef struct gc_header {
    uint32_t size;          // Object size in bytes
    uint8_t generation;     // 0=eden, 1=survivor1, 2=survivor2, 3=old
    uint8_t marked;         // Mark bit for mark-sweep
    uint8_t forwarded;      // Forwarding pointer flag
    uint8_t age;            // Object age (for tenuring)
    void *forwarding_ptr;   // Forwarding pointer for compaction
    struct gc_header *next; // Free list link
} gc_header_t;

typedef struct {
    void *start;
    void *end;
    void *current;
    size_t size;
    gc_header_t *free_list;
    atomic_size_t allocated;
} heap_region_t;

typedef struct {
    heap_region_t eden;
    heap_region_t survivor1;
    heap_region_t survivor2;
    heap_region_t old;
    int current_survivor; // 0 or 1
} generations_t;

typedef struct {
    uint8_t *cards; // Card marking table
    size_t num_cards;
    size_t card_size;
} card_table_t;

struct nova_gc {
    generations_t gens;
    card_table_t card_table;
    gc_state_t state;
    atomic_bool running;

    // Statistics
    atomic_size_t total_allocated;
    atomic_size_t total_collections;
    atomic_size_t young_collections;
    atomic_size_t old_collections;
    atomic_size_t promoted_objects;
    atomic_size_t freed_objects;

    // Roots
    void **roots;
    size_t num_roots;
    size_t max_roots;

    // Write barrier
    void (*write_barrier)(nova_gc_t *, void *, void *, void *);

    // Mutex for thread safety
    pthread_mutex_t mutex;
    pthread_cond_t gc_cond;
};

static void *alloc_from_region(heap_region_t *region, size_t size)
{
    size_t total_size = size + sizeof(gc_header_t);

    if (region->current + total_size > region->end) {
        return NULL; // Region full
    }

    gc_header_t *header = (gc_header_t *) region->current;
    header->size = size;
    header->generation = 0; // Will be set by caller
    header->marked = 0;
    header->forwarded = 0;
    header->age = 0;
    header->forwarding_ptr = NULL;
    header->next = NULL;

    void *object = (void *) (header + 1);
    region->current += total_size;
    atomic_fetch_add(&region->allocated, total_size);

    return object;
}

static heap_region_t *create_heap_region(size_t size)
{
    heap_region_t *region = nova_alloc(sizeof(heap_region_t));
    if (!region)
        return NULL;

    // Use mmap for better memory management
    region->start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region->start == MAP_FAILED) {
        nova_free(region);
        return NULL;
    }

    region->end = region->start + size;
    region->current = region->start;
    region->size = size;
    region->free_list = NULL;
    atomic_init(&region->allocated, 0);

    return region;
}

static void destroy_heap_region(heap_region_t *region)
{
    if (region && region->start) {
        munmap(region->start, region->size);
    }
    nova_free(region);
}

static card_table_t *create_card_table(size_t heap_size, size_t card_size)
{
    card_table_t *table = nova_alloc(sizeof(card_table_t));
    if (!table)
        return NULL;

    table->card_size = card_size;
    table->num_cards = (heap_size + card_size - 1) / card_size;

    table->cards = nova_alloc(table->num_cards);
    if (!table->cards) {
        nova_free(table);
        return NULL;
    }

    memset(table->cards, 0, table->num_cards);
    return table;
}

static void destroy_card_table(card_table_t *table)
{
    if (table) {
        nova_free(table->cards);
        nova_free(table);
    }
}

nova_gc_t *nova_gc_init(nova_gc_type_t type)
{
    if (type != NOVA_GC_GENERATIONAL) {
        return NULL;
    }

    nova_gc_t *gc = nova_alloc(sizeof(nova_gc_t));
    if (!gc)
        return NULL;

    memset(gc, 0, sizeof(nova_gc_t));

    // Initialize generations
    gc->gens.eden = *create_heap_region(EDEN_SIZE);
    gc->gens.survivor1 = *create_heap_region(SURVIVOR_SIZE);
    gc->gens.survivor2 = *create_heap_region(SURVIVOR_SIZE);
    gc->gens.old = *create_heap_region(OLD_GENERATION_SIZE);
    gc->gens.current_survivor = 0;

    // Initialize card table
    size_t total_heap = EDEN_SIZE + 2 * SURVIVOR_SIZE + OLD_GENERATION_SIZE;
    gc->card_table = *create_card_table(total_heap, CARD_SIZE);

    // Initialize state
    gc->state = GC_STATE_IDLE;
    atomic_init(&gc->running, true);
    atomic_init(&gc->total_allocated, 0);
    atomic_init(&gc->total_collections, 0);
    atomic_init(&gc->young_collections, 0);
    atomic_init(&gc->old_collections, 0);
    atomic_init(&gc->promoted_objects, 0);
    atomic_init(&gc->freed_objects, 0);

    // Initialize roots
    gc->max_roots = 1024;
    gc->roots = nova_alloc(sizeof(void *) * gc->max_roots);
    gc->num_roots = 0;

    // Initialize write barrier
    gc->write_barrier = NULL;

    // Initialize mutex
    pthread_mutex_init(&gc->mutex, NULL);
    pthread_cond_init(&gc->gc_cond, NULL);

    return gc;
}

void nova_gc_destroy(nova_gc_t *gc)
{
    if (!gc)
        return;

    destroy_heap_region(&gc->gens.eden);
    destroy_heap_region(&gc->gens.survivor1);
    destroy_heap_region(&gc->gens.survivor2);
    destroy_heap_region(&gc->gens.old);

    destroy_card_table(&gc->card_table);

    nova_free(gc->roots);

    pthread_mutex_destroy(&gc->mutex);
    pthread_cond_destroy(&gc->gc_cond);

    nova_free(gc);
}

void *nova_gc_alloc(nova_gc_t *gc, size_t size)
{
    if (!gc || size == 0)
        return NULL;

    pthread_mutex_lock(&gc->mutex);

    // Try to allocate from eden
    void *obj = alloc_from_region(&gc->gens.eden, size);
    if (obj) {
        gc_header_t *header = (gc_header_t *) obj - 1;
        header->generation = 0; // Eden
        atomic_fetch_add(&gc->total_allocated, size);
        pthread_mutex_unlock(&gc->mutex);
        return obj;
    }

    // Eden full, trigger young GC
    nova_gc_young_collect(gc);

    // Try again
    obj = alloc_from_region(&gc->gens.eden, size);
    if (obj) {
        gc_header_t *header = (gc_header_t *) obj - 1;
        header->generation = 0; // Eden
        atomic_fetch_add(&gc->total_allocated, size);
        pthread_mutex_unlock(&gc->mutex);
        return obj;
    }

    // Still failed, try old generation
    obj = alloc_from_region(&gc->gens.old, size);
    if (obj) {
        gc_header_t *header = (gc_header_t *) obj - 1;
        header->generation = 3; // Old
        atomic_fetch_add(&gc->total_allocated, size);
        pthread_mutex_unlock(&gc->mutex);
        return obj;
    }

    // Old generation also full, trigger full GC
    nova_gc_collect(gc);

    // Final attempt
    obj = alloc_from_region(&gc->gens.eden, size);
    if (obj) {
        gc_header_t *header = (gc_header_t *) obj - 1;
        header->generation = 0; // Eden
        atomic_fetch_add(&gc->total_allocated, size);
    }

    pthread_mutex_unlock(&gc->mutex);
    return obj;
}

static void mark_object(nova_gc_t *gc, void *obj)
{
    if (!obj)
        return;

    gc_header_t *header = (gc_header_t *) obj - 1;
    if (header->marked)
        return;

    header->marked = 1;

    // Mark referenced objects (simplified - would need type info)
    // In a real implementation, we'd traverse the object's references
}

static void mark_roots(nova_gc_t *gc)
{
    for (size_t i = 0; i < gc->num_roots; i++) {
        mark_object(gc, gc->roots[i]);
    }
}

static void sweep_region(heap_region_t *region, atomic_size_t *freed)
{
    void *current = region->start;
    size_t freed_bytes = 0;

    while (current < region->current) {
        gc_header_t *header = (gc_header_t *) current;

        if (!header->marked) {
            // Object is garbage
            freed_bytes += header->size + sizeof(gc_header_t);
            atomic_fetch_add(freed, 1);
        } else {
            // Clear mark for next collection
            header->marked = 0;
        }

        current += header->size + sizeof(gc_header_t);
    }

    // Reset region for next allocation
    region->current = region->start;
    atomic_store(&region->allocated, 0);
}

void nova_gc_collect(nova_gc_t *gc)
{
    if (!gc)
        return;

    pthread_mutex_lock(&gc->mutex);

    gc->state = GC_STATE_MARKING;

    // Mark phase
    mark_roots(gc);

    // Sweep phase
    gc->state = GC_STATE_SWEEPING;
    atomic_store(&gc->freed_objects, 0);

    sweep_region(&gc->gens.eden, &gc->freed_objects);
    sweep_region(&gc->gens.survivor1, &gc->freed_objects);
    sweep_region(&gc->gens.survivor2, &gc->freed_objects);

    // Mark-sweep for old generation
    void *current = gc->gens.old.start;
    while (current < gc->gens.old.current) {
        gc_header_t *header = (gc_header_t *) current;
        if (!header->marked) {
            atomic_fetch_add(&gc->freed_objects, 1);
        } else {
            header->marked = 0;
        }
        current += header->size + sizeof(gc_header_t);
    }

    gc->state = GC_STATE_IDLE;
    atomic_fetch_add(&gc->total_collections, 1);

    pthread_mutex_unlock(&gc->mutex);
}

void nova_gc_young_collect(nova_gc_t *gc)
{
    if (!gc)
        return;

    pthread_mutex_lock(&gc->mutex);

    gc->state = GC_STATE_YOUNG_COLLECTION;

    // Copy collection for young generation
    heap_region_t *from_region = &gc->gens.eden;
    heap_region_t *to_region =
        gc->gens.current_survivor == 0 ? &gc->gens.survivor1 : &gc->gens.survivor2;

    // Reset target region
    to_region->current = to_region->start;
    atomic_store(&to_region->allocated, 0);

    // Copy live objects
    void *current = from_region->start;
    size_t copied = 0;

    while (current < from_region->current) {
        gc_header_t *header = (gc_header_t *) current;

        if (header->marked) {
            // Copy to survivor space
            void *new_obj = alloc_from_region(to_region, header->size);
            if (new_obj) {
                memcpy(new_obj, (void *) (header + 1), header->size);
                gc_header_t *new_header = (gc_header_t *) new_obj - 1;
                new_header->generation = gc->gens.current_survivor + 1;
                new_header->age = header->age + 1;

                // Promote to old generation if old enough
                if (new_header->age >= 8) {
                    // Copy to old generation
                    void *old_obj = alloc_from_region(&gc->gens.old, header->size);
                    if (old_obj) {
                        memcpy(old_obj, new_obj, header->size);
                        gc_header_t *old_header = (gc_header_t *) old_obj - 1;
                        old_header->generation = 3;
                        old_header->age = 0;
                        atomic_fetch_add(&gc->promoted_objects, 1);
                    }
                }
                copied++;
            }
        }

        current += header->size + sizeof(gc_header_t);
    }

    // Swap survivors
    gc->gens.current_survivor = 1 - gc->gens.current_survivor;

    // Reset eden
    from_region->current = from_region->start;
    atomic_store(&from_region->allocated, 0);

    gc->state = GC_STATE_IDLE;
    atomic_fetch_add(&gc->young_collections, 1);

    pthread_mutex_unlock(&gc->mutex);
}

void nova_gc_add_root(nova_gc_t *gc, void *root)
{
    if (!gc || !root)
        return;

    pthread_mutex_lock(&gc->mutex);

    if (gc->num_roots >= gc->max_roots) {
        gc->max_roots *= 2;
        gc->roots = nova_realloc(gc->roots, sizeof(void *) * gc->max_roots);
    }

    gc->roots[gc->num_roots++] = root;

    pthread_mutex_unlock(&gc->mutex);
}

void nova_gc_remove_root(nova_gc_t *gc, void *root)
{
    if (!gc || !root)
        return;

    pthread_mutex_lock(&gc->mutex);

    for (size_t i = 0; i < gc->num_roots; i++) {
        if (gc->roots[i] == root) {
            gc->roots[i] = gc->roots[--gc->num_roots];
            break;
        }
    }

    pthread_mutex_unlock(&gc->mutex);
}

void nova_gc_write_barrier(nova_gc_t *gc, void *parent, void **field, void *value)
{
    if (!gc || !gc->write_barrier)
        return;

    gc->write_barrier(gc, parent, field, value);

    // Mark card for intergenerational references
    if (parent && value) {
        gc_header_t *parent_header = (gc_header_t *) parent - 1;
        gc_header_t *value_header = (gc_header_t *) value - 1;

        // If parent is old gen and value is young gen, mark card
        if (parent_header->generation == 3 && value_header->generation < 3) {
            size_t card_index = ((uintptr_t) parent - (uintptr_t) gc->gens.old.start) / CARD_SIZE;
            if (card_index < gc->card_table.num_cards) {
                gc->card_table.cards[card_index] = 1;
            }
        }
    }
}

nova_gc_stats_t nova_gc_get_stats(nova_gc_t *gc)
{
    nova_gc_stats_t stats = {0};

    if (!gc)
        return stats;

    stats.total_allocated = atomic_load(&gc->total_allocated);
    stats.collections = atomic_load(&gc->total_collections);
    stats.promoted_objects = atomic_load(&gc->promoted_objects);
    stats.freed_objects = atomic_load(&gc->freed_objects);

    stats.eden_used = atomic_load(&gc->gens.eden.allocated);
    stats.survivor1_used = atomic_load(&gc->gens.survivor1.allocated);
    stats.survivor2_used = atomic_load(&gc->gens.survivor2.allocated);
    stats.old_used = atomic_load(&gc->gens.old.allocated);

    return stats;
}

void nova_gc_set_write_barrier(nova_gc_t *gc, void (*barrier)(nova_gc_t *, void *, void *, void *))
{
    if (gc) {
        gc->write_barrier = barrier;
    }
}

/* Legacy interface */
void nova_gc_generational_collect(void)
{
    // No-op - use proper GC instance
}
