/**
 * @file gc.c
 * @brief Garbage collector implementation
 */

#include "runtime/gc.h"
#include "std/alloc.h"
#include <stdlib.h>

struct nova_gc {
    nova_gc_type_t type;
    void *roots[256];
    size_t num_roots;
};

nova_gc_t *nova_gc_init(nova_gc_type_t type)
{
    nova_gc_t *gc = nova_alloc(sizeof(nova_gc_t));
    if (!gc)
        return NULL;

    gc->type = type;
    gc->num_roots = 0;

    return gc;
}

void *nova_gc_alloc(nova_gc_t *gc, size_t size)
{
    (void) gc;
    return nova_alloc(size);
}

void nova_gc_add_root(nova_gc_t *gc, void *ptr)
{
    if (gc->num_roots < 256) {
        gc->roots[gc->num_roots++] = ptr;
    }
}

void nova_gc_destroy(nova_gc_t *gc)
{
    nova_free(gc);
}

void nova_gc_collect(nova_gc_t *gc)
{
    (void) gc;
    // Stub
}
