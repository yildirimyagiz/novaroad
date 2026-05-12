/**
 * @file alloc.c
 * @brief Memory allocation implementation with deterministic tracking
 */

#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Memory statistics (global)
static nova_alloc_stats_t g_alloc_stats = {0};
static bool g_tracking_enabled = true;

// Debug allocation tracking (only in debug builds)
#ifdef NOVA_ALLOC_DEBUG

typedef struct alloc_entry {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    struct alloc_entry* next;
} alloc_entry_t;

static alloc_entry_t* g_alloc_list = NULL;
static size_t g_alloc_count = 0;

static void add_allocation(void* ptr, size_t size, const char* file, int line) {
    if (!g_tracking_enabled) return;
    
    alloc_entry_t* entry = malloc(sizeof(alloc_entry_t));
    if (entry) {
        entry->ptr = ptr;
        entry->size = size;
        entry->file = file;
        entry->line = line;
        entry->next = g_alloc_list;
        g_alloc_list = entry;
        g_alloc_count++;
    }
}

static void remove_allocation(void* ptr) {
    if (!g_tracking_enabled) return;
    
    alloc_entry_t** current = &g_alloc_list;
    while (*current) {
        if ((*current)->ptr == ptr) {
            alloc_entry_t* to_free = *current;
            *current = to_free->next;
            free(to_free);
            g_alloc_count--;
            return;
        }
        current = &(*current)->next;
    }
    
    // Double free detection
    fprintf(stderr, "DOUBLE FREE DETECTED: %p\n", ptr);
    assert(false && "Double free detected");
}

#endif /* NOVA_ALLOC_DEBUG */

void *nova_alloc(size_t size)
{
    if (size == 0) return NULL;
    
    void* ptr = malloc(size);
    if (!ptr) return NULL;
    
    if (g_tracking_enabled) {
        g_alloc_stats.total_allocated += size;
        g_alloc_stats.current_usage += size;
        g_alloc_stats.allocation_count++;
        
        if (g_alloc_stats.current_usage > g_alloc_stats.peak_usage) {
            g_alloc_stats.peak_usage = g_alloc_stats.current_usage;
        }
    }
    
#ifdef NOVA_ALLOC_DEBUG
    add_allocation(ptr, size, __FILE__, __LINE__);
#endif
    
    return ptr;
}

void *nova_calloc(size_t count, size_t size)
{
    if (count == 0 || size == 0) return NULL;
    
    void* ptr = calloc(count, size);
    if (!ptr) return NULL;
    
    size_t total_size = count * size;
    
    if (g_tracking_enabled) {
        g_alloc_stats.total_allocated += total_size;
        g_alloc_stats.current_usage += total_size;
        g_alloc_stats.allocation_count++;
        
        if (g_alloc_stats.current_usage > g_alloc_stats.peak_usage) {
            g_alloc_stats.peak_usage = g_alloc_stats.current_usage;
        }
    }
    
#ifdef NOVA_ALLOC_DEBUG
    add_allocation(ptr, total_size, __FILE__, __LINE__);
#endif
    
    return ptr;
}

void *nova_realloc(void *ptr, size_t new_size)
{
    if (new_size == 0) {
        nova_free(ptr);
        return NULL;
    }
    
    if (!ptr) {
        return nova_alloc(new_size);
    }
    
    // Get old size for tracking
    size_t old_size = 0;
#ifdef NOVA_ALLOC_DEBUG
    alloc_entry_t* current = g_alloc_list;
    while (current) {
        if (current->ptr == ptr) {
            old_size = current->size;
            break;
        }
        current = current->next;
    }
#endif
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) return NULL;
    
    if (g_tracking_enabled && old_size > 0) {
        g_alloc_stats.current_usage -= old_size;
        g_alloc_stats.current_usage += new_size;
        g_alloc_stats.total_freed += old_size;
        g_alloc_stats.total_allocated += new_size;
        
        if (g_alloc_stats.current_usage > g_alloc_stats.peak_usage) {
            g_alloc_stats.peak_usage = g_alloc_stats.current_usage;
        }
    }
    
#ifdef NOVA_ALLOC_DEBUG
    remove_allocation(ptr);
    add_allocation(new_ptr, new_size, __FILE__, __LINE__);
#endif
    
    return new_ptr;
}

void nova_free(void *ptr)
{
    if (!ptr) return;
    
    // Get size for tracking
    size_t freed_size = 0;
#ifdef NOVA_ALLOC_DEBUG
    alloc_entry_t* current = g_alloc_list;
    while (current) {
        if (current->ptr == ptr) {
            freed_size = current->size;
            break;
        }
        current = current->next;
    }
#endif
    
    free(ptr);
    
    if (g_tracking_enabled && freed_size > 0) {
        g_alloc_stats.total_freed += freed_size;
        g_alloc_stats.current_usage -= freed_size;
        g_alloc_stats.free_count++;
    }
    
#ifdef NOVA_ALLOC_DEBUG
    remove_allocation(ptr);
#endif
}

size_t nova_alloc_size(void *ptr)
{
    if (!ptr) return 0;
    
#ifdef NOVA_ALLOC_DEBUG
    alloc_entry_t* current = g_alloc_list;
    while (current) {
        if (current->ptr == ptr) {
            return current->size;
        }
        current = current->next;
    }
#endif
    
    return 0; // Unknown size
}

void *nova_memdup(const void *ptr, size_t size)
{
    if (!ptr || size == 0) return NULL;
    
    void* dup = nova_alloc(size);
    if (dup) {
        memcpy(dup, ptr, size);
    }
    return dup;
}

void *nova_aligned_alloc(size_t alignment, size_t size)
{
    if (alignment == 0 || size == 0) return NULL;
    
    void* ptr;
    
#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
#else
    ptr = aligned_alloc(alignment, size);
#endif
    
    if (!ptr) return NULL;
    
    if (g_tracking_enabled) {
        g_alloc_stats.total_allocated += size;
        g_alloc_stats.current_usage += size;
        g_alloc_stats.allocation_count++;
        
        if (g_alloc_stats.current_usage > g_alloc_stats.peak_usage) {
            g_alloc_stats.peak_usage = g_alloc_stats.current_usage;
        }
    }
    
#ifdef NOVA_ALLOC_DEBUG
    add_allocation(ptr, size, __FILE__, __LINE__);
#endif
    
    return ptr;
}

void nova_aligned_free(void *ptr)
{
    if (!ptr) return;
    
    // Get size for tracking
    size_t freed_size = 0;
#ifdef NOVA_ALLOC_DEBUG
    alloc_entry_t* current = g_alloc_list;
    while (current) {
        if (current->ptr == ptr) {
            freed_size = current->size;
            break;
        }
        current = current->next;
    }
#endif
    
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
    
    if (g_tracking_enabled && freed_size > 0) {
        g_alloc_stats.total_freed += freed_size;
        g_alloc_stats.current_usage -= freed_size;
        g_alloc_stats.free_count++;
    }
    
#ifdef NOVA_ALLOC_DEBUG
    remove_allocation(ptr);
#endif
}

bool nova_is_aligned(const void *ptr, size_t alignment)
{
    if (!ptr || alignment == 0) return false;
    return ((uintptr_t)ptr % alignment) == 0;
}

void nova_alloc_get_stats(nova_alloc_stats_t *stats)
{
    if (stats) {
        *stats = g_alloc_stats;
    }
}

void nova_alloc_reset_stats(void)
{
    memset(&g_alloc_stats, 0, sizeof(g_alloc_stats));
}

void nova_alloc_set_tracking(bool enabled)
{
    g_tracking_enabled = enabled;
}

#ifdef NOVA_ALLOC_DEBUG

size_t nova_alloc_check_leaks(void)
{
    size_t leaks = 0;
    alloc_entry_t* current = g_alloc_list;
    
    while (current) {
        fprintf(stderr, "MEMORY LEAK: %p (%zu bytes) at %s:%d\n", 
                current->ptr, current->size, current->file, current->line);
        leaks++;
        current = current->next;
    }
    
    return leaks;
}

void nova_alloc_dump(void)
{
    fprintf(stderr, "=== NOVA MEMORY DUMP ===\n");
    fprintf(stderr, "Active allocations: %zu\n", g_alloc_count);
    fprintf(stderr, "Total allocated: %zu bytes\n", g_alloc_stats.total_allocated);
    fprintf(stderr, "Total freed: %zu bytes\n", g_alloc_stats.total_freed);
    fprintf(stderr, "Current usage: %zu bytes\n", g_alloc_stats.current_usage);
    fprintf(stderr, "Peak usage: %zu bytes\n", g_alloc_stats.peak_usage);
    
    alloc_entry_t* current = g_alloc_list;
    while (current) {
        fprintf(stderr, "  %p: %zu bytes (%s:%d)\n", 
                current->ptr, current->size, current->file, current->line);
        current = current->next;
    }
    fprintf(stderr, "========================\n");
}

#endif /* NOVA_ALLOC_DEBUG */
