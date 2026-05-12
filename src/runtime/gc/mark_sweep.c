/**
 * @file mark_sweep.c
 * @brief Mark-sweep garbage collector with tri-color marking
 */

#include "runtime/gc.h"
#include "std/alloc.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Object header for mark-sweep
typedef struct ms_header {
    uint32_t size;          // Object size in bytes
    uint8_t color;          // Tri-color marking: 0=white, 1=gray, 2=black
    uint8_t marked;         // Mark bit for mark-sweep
    uint8_t generation;     // Generation for generational GC
    uint8_t flags;          // Additional flags
    struct ms_header *next; // Free list link
} ms_header_t;

// Color definitions for tri-color marking
#define COLOR_WHITE 0
#define COLOR_GRAY 1
#define COLOR_BLACK 2

// Memory block for allocation
typedef struct memory_block {
    void *start;
    void *end;
    size_t size;
    struct memory_block *next;
} memory_block_t;

// Work list for gray objects (tri-color marking)
typedef struct work_list {
    void **objects;
    size_t count;
    size_t capacity;
} work_list_t;

typedef struct nova_mark_sweep_gc nova_mark_sweep_gc_t;

struct nova_mark_sweep_gc {
    memory_block_t *blocks; // Memory blocks
    ms_header_t *free_list; // Free object list
    work_list_t work_list;  // Gray objects to process

    void **roots; // GC roots
    size_t num_roots;
    size_t max_roots;

    // Statistics
    size_t total_allocated;
    size_t total_collections;
    size_t objects_marked;
    size_t objects_swept;
    size_t bytes_freed;

    // Threading
    pthread_mutex_t gc_mutex;
    atomic_bool collecting;
};

static const size_t BLOCK_SIZE = 1024 * 1024; // 1MB blocks
static const size_t MIN_OBJECT_SIZE = sizeof(ms_header_t) + 16;

static memory_block_t *allocate_block(size_t size)
{
    memory_block_t *block = malloc(sizeof(memory_block_t));
    if (!block)
        return NULL;

    block->size = size;
    block->start = malloc(size);
    if (!block->start) {
        free(block);
        return NULL;
    }

    block->end = (char *) block->start + size;
    block->next = NULL;

    return block;
}

static void free_block(memory_block_t *block)
{
    if (block) {
        free(block->start);
        free(block);
    }
}

static work_list_t *work_list_create(size_t capacity)
{
    work_list_t *list = malloc(sizeof(work_list_t));
    if (!list)
        return NULL;

    list->objects = malloc(sizeof(void *) * capacity);
    if (!list->objects) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = capacity;
    return list;
}

static void work_list_destroy(work_list_t *list)
{
    if (list) {
        free(list->objects);
        free(list);
    }
}

static int work_list_push(work_list_t *list, void *obj)
{
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        void **new_objects = realloc(list->objects, sizeof(void *) * new_capacity);
        if (!new_objects)
            return -1;

        list->objects = new_objects;
        list->capacity = new_capacity;
    }

    list->objects[list->count++] = obj;
    return 0;
}

static void *work_list_pop(work_list_t *list)
{
    if (list->count == 0)
        return NULL;
    return list->objects[--list->count];
}

nova_mark_sweep_gc_t *nova_mark_sweep_gc_create(void)
{
    nova_mark_sweep_gc_t *gc = malloc(sizeof(nova_mark_sweep_gc_t));
    if (!gc)
        return NULL;

    memset(gc, 0, sizeof(nova_mark_sweep_gc_t));

    // Allocate initial memory block
    gc->blocks = allocate_block(BLOCK_SIZE);
    if (!gc->blocks) {
        free(gc);
        return NULL;
    }

    // Initialize work list
    gc->work_list = *work_list_create(1024);
    if (!gc->work_list.objects) {
        free_block(gc->blocks);
        free(gc);
        return NULL;
    }

    // Initialize roots
    gc->max_roots = 1024;
    gc->roots = malloc(sizeof(void *) * gc->max_roots);
    if (!gc->roots) {
        work_list_destroy(&gc->work_list);
        free_block(gc->blocks);
        free(gc);
        return NULL;
    }

    gc->num_roots = 0;
    pthread_mutex_init(&gc->gc_mutex, NULL);
    atomic_init(&gc->collecting, false);

    return gc;
}

void nova_mark_sweep_gc_destroy(nova_mark_sweep_gc_t *gc)
{
    if (!gc)
        return;

    // Free all memory blocks
    memory_block_t *block = gc->blocks;
    while (block) {
        memory_block_t *next = block->next;
        free_block(block);
        block = next;
    }

    work_list_destroy(&gc->work_list);
    free(gc->roots);
    pthread_mutex_destroy(&gc->gc_mutex);
    free(gc);
}

void *nova_mark_sweep_gc_alloc(nova_mark_sweep_gc_t *gc, size_t size)
{
    if (!gc || size == 0)
        return NULL;

    size_t total_size = sizeof(ms_header_t) + size;
    if (total_size < MIN_OBJECT_SIZE) {
        total_size = MIN_OBJECT_SIZE;
    }

    pthread_mutex_lock(&gc->gc_mutex);

    // Try to allocate from free list first
    ms_header_t *header = gc->free_list;
    ms_header_t *prev = NULL;

    while (header) {
        if (header->size >= total_size) {
            // Remove from free list
            if (prev) {
                prev->next = header->next;
            } else {
                gc->free_list = header->next;
            }

            // Initialize object
            header->color = COLOR_WHITE;
            header->marked = 0;
            header->generation = 0;
            header->flags = 0;

            gc->total_allocated += total_size;
            void *object = (char *) header + sizeof(ms_header_t);

            pthread_mutex_unlock(&gc->gc_mutex);
            return object;
        }
        prev = header;
        header = header->next;
    }

    // No suitable free object, allocate from current block
    memory_block_t *block = gc->blocks;

    // Find the last block
    while (block->next) {
        block = block->next;
    }

    // Check if there's space in current block
    size_t used = (char *) block->end - (char *) block->start;
    if (used + total_size > block->size) {
        // Allocate new block
        memory_block_t *new_block = allocate_block(BLOCK_SIZE);
        if (!new_block) {
            pthread_mutex_unlock(&gc->gc_mutex);
            return NULL;
        }

        block->next = new_block;
        block = new_block;
    }

    // Allocate from block
    header = (ms_header_t *) ((char *) block->start + used);
    header->size = total_size;
    header->color = COLOR_WHITE;
    header->marked = 0;
    header->generation = 0;
    header->flags = 0;
    header->next = NULL;

    gc->total_allocated += total_size;

    void *object = (char *) header + sizeof(ms_header_t);

    pthread_mutex_unlock(&gc->gc_mutex);
    return object;
}

static void mark_object(nova_mark_sweep_gc_t *gc, void *obj)
{
    if (!obj)
        return;

    ms_header_t *header = (ms_header_t *) ((char *) obj - sizeof(ms_header_t));

    if (header->color == COLOR_WHITE) {
        header->color = COLOR_GRAY;
        header->marked = 1;
        gc->objects_marked++;

        // Add to work list for further processing
        work_list_push(&gc->work_list, obj);
    }
}

static void mark_roots(nova_mark_sweep_gc_t *gc)
{
    for (size_t i = 0; i < gc->num_roots; i++) {
        mark_object(gc, gc->roots[i]);
    }
}

static void process_work_list(nova_mark_sweep_gc_t *gc)
{
    while (1) {
        void *obj = work_list_pop(&gc->work_list);
        if (!obj)
            break;

        ms_header_t *header = (ms_header_t *) ((char *) obj - sizeof(ms_header_t));

        if (header->color == COLOR_GRAY) {
            header->color = COLOR_BLACK;

            // Mark referenced objects (simplified - would need type info)
            // In a real implementation, we'd traverse the object's references
        }
    }
}

static void sweep_phase(nova_mark_sweep_gc_t *gc)
{
    gc->bytes_freed = 0;
    gc->objects_swept = 0;

    // Sweep all memory blocks
    memory_block_t *block = gc->blocks;
    while (block) {
        void *current = block->start;

        while (current < block->end) {
            ms_header_t *header = (ms_header_t *) current;

            if (header->color == COLOR_WHITE && header->marked == 0) {
                // Object is garbage, add to free list
                header->next = gc->free_list;
                gc->free_list = header;

                gc->bytes_freed += header->size;
                gc->objects_swept++;
            } else {
                // Reset for next collection
                header->color = COLOR_WHITE;
                header->marked = 0;
            }

            current = (char *) current + header->size;
        }

        block = block->next;
    }
}

void nova_mark_sweep_gc_collect(nova_mark_sweep_gc_t *gc)
{
    if (!gc)
        return;

    // Prevent concurrent collections
    if (atomic_exchange(&gc->collecting, true)) {
        return; // Already collecting
    }

    pthread_mutex_lock(&gc->gc_mutex);

    // Mark phase
    mark_roots(gc);
    process_work_list(gc);

    // Sweep phase
    sweep_phase(gc);

    gc->total_collections++;

    pthread_mutex_unlock(&gc->gc_mutex);
    atomic_store(&gc->collecting, false);
}

void nova_mark_sweep_gc_add_root(nova_mark_sweep_gc_t *gc, void *root)
{
    if (!gc || !root)
        return;

    pthread_mutex_lock(&gc->gc_mutex);

    if (gc->num_roots >= gc->max_roots) {
        gc->max_roots *= 2;
        void **new_roots = realloc(gc->roots, sizeof(void *) * gc->max_roots);
        if (!new_roots) {
            pthread_mutex_unlock(&gc->gc_mutex);
            return;
        }
        gc->roots = new_roots;
    }

    gc->roots[gc->num_roots++] = root;

    pthread_mutex_unlock(&gc->gc_mutex);
}

void nova_mark_sweep_gc_remove_root(nova_mark_sweep_gc_t *gc, void *root)
{
    if (!gc || !root)
        return;

    pthread_mutex_lock(&gc->gc_mutex);

    for (size_t i = 0; i < gc->num_roots; i++) {
        if (gc->roots[i] == root) {
            gc->roots[i] = gc->roots[--gc->num_roots];
            break;
        }
    }

    pthread_mutex_unlock(&gc->gc_mutex);
}

nova_gc_stats_t nova_mark_sweep_gc_stats(nova_mark_sweep_gc_t *gc)
{
    nova_gc_stats_t stats = {0};

    if (!gc)
        return stats;

    stats.heap_used = gc->total_allocated;
    stats.collections_total = gc->total_collections;
    stats.objects_live = gc->objects_marked;
    stats.objects_freed = gc->objects_swept;
    stats.heap_size = gc->total_allocated; // Simplified

    return stats;
}

int nova_mark_sweep_gc_is_collecting(nova_mark_sweep_gc_t *gc)
{
    return gc ? atomic_load(&gc->collecting) : 0;
}

size_t nova_mark_sweep_gc_pressure(nova_mark_sweep_gc_t *gc)
{
    if (!gc)
        return 0;

    // Calculate memory pressure (simplified)
    // In a real implementation, this would be based on allocation rate vs collection rate
    return gc->total_allocated / (gc->total_collections + 1);
}
