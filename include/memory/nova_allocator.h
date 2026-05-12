#ifndef NOVA_ALLOCATOR_H
#define NOVA_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ALIGNMENT-AWARE ALLOCATOR
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Memory tags for tracking
typedef enum {
  MEM_TAG_GENERAL,
  MEM_TAG_TENSOR, // Aligned for SIMD/GPU
  MEM_TAG_CODE,   // JIT/Executable
  MEM_TAG_BACKEND // Backend internal usage
} MemoryTag;

// Alignment constants
#define NOVA_CACHE_LINE_SIZE 64
#define NOVA_SIMD_ALIGNMENT 64 // AVX-512 aligned

void *nova_malloc(size_t size, MemoryTag tag);
void *nova_malloc_aligned(size_t size, size_t alignment, MemoryTag tag);
void nova_free(void *ptr);

// Pool management
void *nova_global_pool_alloc(size_t size);
void nova_global_pool_reset(void);

#endif // NOVA_ALLOCATOR_H
