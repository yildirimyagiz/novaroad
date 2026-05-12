/**
 * @file arena.c
 * @brief High-performance arena allocator with linked arenas
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

#define DEFAULT_ARENA_SIZE (2 * 1024 * 1024) // 2MB per arena
#define MIN_ARENA_SIZE (64 * 1024)           // 64KB minimum
#define MAX_ARENA_SIZE (64 * 1024 * 1024)    // 64MB maximum
#define ALIGNMENT 16                         // 16-byte alignment

// Arena header stored at the beginning of each arena
typedef struct arena_header {
    size_t size;               // Total arena size
    size_t used;               // Bytes currently used
    size_t committed;          // Bytes committed to memory
    struct arena_header *next; // Next arena in chain
    struct arena_header *prev; // Previous arena in chain
    atomic_int ref_count;      // Reference counting
    void *data_start;          // Start of allocation area
    pthread_mutex_t mutex;     // Thread safety
} arena_header_t;

typedef struct {
    size_t total_used;
    size_t peak_used;
    size_t arena_count;
    size_t default_size;
    double efficiency;
} nova_arena_stats_t;

typedef struct nova_arena nova_arena_t;

struct nova_arena {
    arena_header_t *current;     // Current arena for allocations
    size_t default_size;         // Default arena size
    size_t total_allocated;      // Total bytes allocated across all arenas
    size_t peak_usage;           // Peak memory usage
    atomic_size_t arena_count;   // Number of arenas
    pthread_mutex_t arena_mutex; // Protects arena list
};

static size_t align_size(size_t size)
{
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

static arena_header_t *create_arena(size_t size)
{
    // Align size to page boundary for better mmap usage
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    // Allocate space for header + data
    size_t total_size = sizeof(arena_header_t) + aligned_size;

    // Use mmap for large allocations to reduce fragmentation
    void *memory =
        mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }

    arena_header_t *header = (arena_header_t *) memory;
    memset(header, 0, sizeof(arena_header_t));

    header->size = aligned_size;
    header->used = 0;
    header->committed = 0;
    header->data_start = (char *) memory + sizeof(arena_header_t);
    atomic_init(&header->ref_count, 1);
    pthread_mutex_init(&header->mutex, NULL);

    return header;
}

static void destroy_arena(arena_header_t *arena)
{
    if (!arena)
        return;

    pthread_mutex_destroy(&arena->mutex);
    munmap(arena, sizeof(arena_header_t) + arena->size);
}

nova_arena_t *nova_arena_create(size_t initial_size)
{
    if (initial_size == 0) {
        initial_size = DEFAULT_ARENA_SIZE;
    }

    // Clamp size to reasonable bounds
    if (initial_size < MIN_ARENA_SIZE) {
        initial_size = MIN_ARENA_SIZE;
    }
    if (initial_size > MAX_ARENA_SIZE) {
        initial_size = MAX_ARENA_SIZE;
    }

    nova_arena_t *arena = nova_alloc(sizeof(nova_arena_t));
    if (!arena)
        return NULL;

    memset(arena, 0, sizeof(nova_arena_t));

    arena->default_size = initial_size;
    arena->current = create_arena(initial_size);
    if (!arena->current) {
        nova_free(arena);
        return NULL;
    }

    atomic_init(&arena->arena_count, 1);
    pthread_mutex_init(&arena->arena_mutex, NULL);

    return arena;
}

void nova_arena_destroy(nova_arena_t *arena)
{
    if (!arena)
        return;

    // Free all arenas in the chain
    arena_header_t *current = arena->current;
    while (current) {
        arena_header_t *next = current->next;
        destroy_arena(current);
        current = next;
    }

    pthread_mutex_destroy(&arena->arena_mutex);
    nova_free(arena);
}

void *nova_arena_alloc(nova_arena_t *arena, size_t size)
{
    if (!arena || size == 0)
        return NULL;

    size = align_size(size);

    pthread_mutex_lock(&arena->arena_mutex);

    // Try to allocate from current arena
    arena_header_t *current = arena->current;
    if (current->used + size <= current->size) {
        void *ptr = (char *) current->data_start + current->used;
        current->used += size;
        arena->total_allocated += size;

        // Update peak usage
        if (arena->total_allocated > arena->peak_usage) {
            arena->peak_usage = arena->total_allocated;
        }

        pthread_mutex_unlock(&arena->arena_mutex);
        return ptr;
    }

    // Current arena is full, create new arena
    size_t new_size = arena->default_size;
    if (size > new_size) {
        new_size = size; // Allocate exactly what's needed
    }

    arena_header_t *new_arena = create_arena(new_size);
    if (!new_arena) {
        pthread_mutex_unlock(&arena->arena_mutex);
        return NULL;
    }

    // Link new arena to the chain
    new_arena->prev = current;
    current->next = new_arena;
    arena->current = new_arena;
    atomic_fetch_add(&arena->arena_count, 1);

    // Allocate from new arena
    void *ptr = (char *) new_arena->data_start + new_arena->used;
    new_arena->used += size;
    arena->total_allocated += size;

    pthread_mutex_unlock(&arena->arena_mutex);
    return ptr;
}

void *nova_arena_calloc(nova_arena_t *arena, size_t nmemb, size_t size)
{
    if (!arena || nmemb == 0 || size == 0)
        return NULL;

    size_t total_size = nmemb * size;
    void *ptr = nova_arena_alloc(arena, total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void *nova_arena_realloc(nova_arena_t *arena, void *ptr, size_t old_size, size_t new_size)
{
    if (!arena)
        return NULL;
    if (!ptr)
        return nova_arena_alloc(arena, new_size);
    if (new_size == 0)
        return NULL;

    new_size = align_size(new_size);

    // For arena allocators, we can't actually shrink or move existing allocations
    // So we allocate new space and copy data
    void *new_ptr = nova_arena_alloc(arena, new_size);
    if (!new_ptr)
        return NULL;

    size_t copy_size = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);

    return new_ptr;
}

void nova_arena_reset(nova_arena_t *arena)
{
    if (!arena)
        return;

    pthread_mutex_lock(&arena->arena_mutex);

    // Reset all arenas to their initial state
    arena_header_t *current = arena->current;
    while (current) {
        current->used = 0;
        current = current->prev; // Go backwards to reset all
    }

    // Reset to first arena
    while (arena->current->prev) {
        arena->current = arena->current->prev;
    }

    arena->total_allocated = 0;

    pthread_mutex_unlock(&arena->arena_mutex);
}

size_t nova_arena_used(nova_arena_t *arena)
{
    return arena ? arena->total_allocated : 0;
}

size_t nova_arena_peak(nova_arena_t *arena)
{
    return arena ? arena->peak_usage : 0;
}

size_t nova_arena_count(nova_arena_t *arena)
{
    return arena ? atomic_load(&arena->arena_count) : 0;
}

nova_arena_stats_t nova_arena_stats(nova_arena_t *arena)
{
    nova_arena_stats_t stats = {0};

    if (!arena)
        return stats;

    stats.total_used = arena->total_allocated;
    stats.peak_used = arena->peak_usage;
    stats.arena_count = atomic_load(&arena->arena_count);
    stats.default_size = arena->default_size;

    // Calculate efficiency
    size_t total_space = stats.arena_count * arena->default_size;
    if (total_space > 0) {
        stats.efficiency = (double) stats.total_used / (double) total_space * 100.0;
    }

    return stats;
}

void nova_arena_compact(nova_arena_t *arena)
{
    if (!arena)
        return;

    pthread_mutex_lock(&arena->arena_mutex);

    // Find the last arena with allocations
    arena_header_t *last_used = NULL;
    arena_header_t *current = arena->current;

    while (current) {
        if (current->used > 0) {
            last_used = current;
        }
        current = current->prev;
    }

    if (!last_used) {
        // No allocations, reset to first arena
        nova_arena_reset(arena);
        pthread_mutex_unlock(&arena->arena_mutex);
        return;
    }

    // Move current to last used arena
    arena->current = last_used;

    // Free unused arenas after last_used
    current = last_used->next;
    while (current) {
        arena_header_t *next = current->next;
        destroy_arena(current);
        atomic_fetch_sub(&arena->arena_count, 1);
        current = next;
    }

    last_used->next = NULL;

    pthread_mutex_unlock(&arena->arena_mutex);
}

nova_arena_t *nova_arena_clone(nova_arena_t *arena)
{
    if (!arena)
        return NULL;

    nova_arena_t *clone = nova_alloc(sizeof(nova_arena_t));
    if (!clone)
        return NULL;

    memcpy(clone, arena, sizeof(nova_arena_t));

    // Increment reference counts for all arenas
    arena_header_t *current = clone->current;
    while (current) {
        atomic_fetch_add(&current->ref_count, 1);
        current = current->prev;
    }

    return clone;
}

int nova_arena_contains(nova_arena_t *arena, void *ptr)
{
    if (!arena || !ptr)
        return 0;

    // Check if pointer is within any of our arenas
    arena_header_t *current = arena->current;
    while (current) {
        void *arena_start = current->data_start;
        void *arena_end = (char *) arena_start + current->size;

        if (ptr >= arena_start && ptr < arena_end) {
            return 1;
        }

        current = current->prev;
    }

    return 0;
}
