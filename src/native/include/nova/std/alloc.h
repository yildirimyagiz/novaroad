/**
 * @file alloc.h
 * @brief Memory allocation utilities for Nova
 */

#ifndef NOVA_STD_ALLOC_H
#define NOVA_STD_ALLOC_H

#include <stddef.h>

/**
 * Allocate memory
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL on error
 */
void *nova_alloc(size_t size);

/**
 * Reallocate memory
 * @param ptr Pointer to existing allocation
 * @param size New size in bytes
 * @return Pointer to reallocated memory or NULL on error
 */
void *nova_realloc(void *ptr, size_t size);

/**
 * Free allocated memory
 * @param ptr Pointer to free
 */
void nova_free(void *ptr);

/**
 * Allocate and zero memory
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to zeroed memory or NULL on error
 */
void *nova_calloc(size_t count, size_t size);

#endif /* NOVA_STD_ALLOC_H */
