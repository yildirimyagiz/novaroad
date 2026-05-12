/**
 * @file heap.c
 * @brief Kernel heap allocator
 */

#include "heap.h"
#include "kernel/memory.h"
#include <stddef.h>

/* Simple bump allocator for now */
#define HEAP_SIZE (1024 * 1024) /* 1MB */

static struct {
    uint8_t data[HEAP_SIZE];
    size_t offset;
} kernel_heap;

void nova_heap_init(void)
{
    kernel_heap.offset = 0;
}

void *nova_kmalloc(size_t size)
{
    if (kernel_heap.offset + size > HEAP_SIZE) {
        return NULL; /* Out of memory */
    }
    
    void *ptr = &kernel_heap.data[kernel_heap.offset];
    kernel_heap.offset += size;
    
    /* Align to 8 bytes */
    kernel_heap.offset = (kernel_heap.offset + 7) & ~7;
    
    return ptr;
}

void nova_kfree(void *ptr)
{
    /* TODO: Implement proper free */
    (void)ptr;
}
