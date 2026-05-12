/**
 * @file dma.c
 * @brief DMA memory management
 */

#include "dma.h"
#include "kernel/memory.h"

void *nova_dma_alloc(size_t size)
{
    /* TODO: Allocate DMA-capable memory */
    (void)size;
    return NULL;
}

void nova_dma_free(void *ptr)
{
    /* TODO: Free DMA memory */
    (void)ptr;
}
