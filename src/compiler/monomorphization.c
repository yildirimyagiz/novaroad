/**
 * @file monomorphization.c
 * @brief Generic monomorphization implementation
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "std/alloc.h"


typedef struct mono_instance {
    void *generic_base;
    void **type_args;
    int type_arg_count;
    void *specialized_instance;
    struct mono_instance *next;
} mono_instance_t;

static mono_instance_t *g_mono_cache = NULL;

// Helper: Check if type args match
static bool type_args_match(void **a, void **b, int count)
{
    for (int i = 0; i < count; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Get or create monomorphic instance
void *get_monomorphic_instance(void *generic, void **type_args, int count)
{
    mono_instance_t *cur = g_mono_cache;
    while (cur) {
        if (cur->generic_base == generic && cur->type_arg_count == count) {
            if (type_args_match(cur->type_args, type_args, count)) {
                return cur->specialized_instance;
            }
        }
        cur = cur->next;
    }

    // Not found, create new instance
    mono_instance_t *entry = nova_alloc(sizeof(mono_instance_t));
    entry->generic_base = generic;
    entry->type_arg_count = count;
    entry->type_args = nova_alloc(sizeof(void*) * count);
    memcpy(entry->type_args, type_args, sizeof(void*) * count);
    
    // In a real compiler, we would trigger a re-compilation of the generic body
    // with these type arguments here.
    entry->specialized_instance = generic; // Placeholder: Return base for now
    
    entry->next = g_mono_cache;
    g_mono_cache = entry;
    
    return entry->specialized_instance;
}

// Monomorphize generic function
void *monomorphize_function(void *generic_func, void **type_args, int count)
{
    return get_monomorphic_instance(generic_func, type_args, count);
}

// Monomorphize generic type
void *monomorphize_type(void *generic_type, void **type_args, int count)
{
    return get_monomorphic_instance(generic_type, type_args, count);
}

// Clear monomorphization cache
void clear_mono_cache(void)
{
    mono_instance_t *cur = g_mono_cache;
    while (cur) {
        mono_instance_t *next = cur->next;
        nova_free(cur->type_args);
        nova_free(cur);
        cur = next;
    }
    g_mono_cache = NULL;
}

