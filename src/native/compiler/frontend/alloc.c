/**
 * @file alloc.c
 * @brief Memory allocation implementation
 */

#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>

void *nova_alloc(size_t size)
{
    return malloc(size);
}

void *nova_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void nova_free(void *ptr)
{
    free(ptr);
}

void *nova_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}
