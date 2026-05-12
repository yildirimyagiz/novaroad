/**
 * @file dma.h
 * @brief DMA internal definitions
 */

#ifndef NOVA_KERNEL_DMA_H
#define NOVA_KERNEL_DMA_H

#include <stddef.h>

void *nova_dma_alloc(size_t size);
void nova_dma_free(void *ptr);

#endif /* NOVA_KERNEL_DMA_H */
