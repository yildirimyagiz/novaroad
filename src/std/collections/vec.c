/**
 * @file vec.c
 * @brief Dynamic array implementation
 */

#include "std/collections.h"
#include "std/alloc.h"

struct nova_vec {
    void **items;
    size_t length;
    size_t capacity;
};

nova_vec_t *nova_vec_new(void)
{
    nova_vec_t *vec = nova_alloc(sizeof(nova_vec_t));
    if (!vec) return NULL;
    
    vec->capacity = 16;
    vec->length = 0;
    vec->items = nova_alloc(sizeof(void *) * vec->capacity);
    
    return vec;
}

bool nova_vec_push(nova_vec_t *vec, void *item)
{
    if (vec->length >= vec->capacity) {
        vec->capacity *= 2;
        vec->items = nova_realloc(vec->items, sizeof(void *) * vec->capacity);
        if (!vec->items) return false;
    }
    
    vec->items[vec->length++] = item;
    return true;
}

void *nova_vec_pop(nova_vec_t *vec)
{
    if (vec->length == 0) return NULL;
    return vec->items[--vec->length];
}

void *nova_vec_get(nova_vec_t *vec, size_t index)
{
    if (index >= vec->length) return NULL;
    return vec->items[index];
}

size_t nova_vec_len(nova_vec_t *vec)
{
    return vec->length;
}

void nova_vec_destroy(nova_vec_t *vec)
{
    if (vec) {
        nova_free(vec->items);
        nova_free(vec);
    }
}

void nova_vec_clear(nova_vec_t *vec)
{
    if (vec) {
        vec->length = 0;
    }
}
