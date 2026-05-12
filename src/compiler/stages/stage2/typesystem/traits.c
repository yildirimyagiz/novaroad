/**
 * @file traits.c
 * @brief Trait/interface resolution
 */

#include "compiler/nova_types.h"
#include "std/alloc.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>


typedef struct {
    const char *trait_name;
    const char **method_names;
    size_t num_methods;
} trait_t;

typedef struct impl_entry {
    nova_type_t *type;
    const char *trait_name;
    void **methods;  // Array of function pointers
    struct impl_entry *next;
} impl_entry_t;

static impl_entry_t *g_impl_registry = NULL;

/* Register a trait implementation for a type */
void nova_register_trait_impl(nova_type_t *type, const char *trait_name, void **methods)
{
    impl_entry_t *entry = nova_alloc(sizeof(impl_entry_t));
    entry->type = type;
    entry->trait_name = strdup(trait_name);
    entry->methods = methods;
    entry->next = g_impl_registry;
    g_impl_registry = entry;
}

/* Check if type implements trait */
bool nova_type_implements_trait(nova_type_t *type, const char *trait_name)
{
    impl_entry_t *cur = g_impl_registry;
    while (cur) {
        if (strcmp(cur->trait_name, trait_name) == 0 && nova_type_equals(cur->type, type)) {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

/* Resolve trait method */
void *nova_resolve_trait_method(nova_type_t *type, const char *trait_name, const char *method_name)
{
    impl_entry_t *cur = g_impl_registry;
    while (cur) {
        if (strcmp(cur->trait_name, trait_name) == 0 && nova_type_equals(cur->type, type)) {
            // In a real implementation, we would look up the method index in the trait definition
            // and return cur->methods[index]. For now, return the first method as a stub.
            return cur->methods[0];
        }
        cur = cur->next;
    }
    return NULL;
}

