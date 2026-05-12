#include "../../include/nova_allocator.h"
#include <stdio.h>
#include <stdlib.h>

void *nova_malloc(size_t size, MemoryTag tag) {
  // General allocation
  return nova_malloc_aligned(size, 16, tag);
}

void *nova_malloc_aligned(size_t size, size_t alignment, MemoryTag tag) {
  (void)tag;
  void *ptr = NULL;

#ifdef _WIN32
  ptr = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&ptr, alignment, size) != 0) {
    return NULL;
  }
#endif

  // In a full implementation, we would track allocation by tag here
  return ptr;
}

void nova_free(void *ptr) {
  if (!ptr)
    return;
#ifdef _WIN32
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

// Simple linear pool for high-frequency allocations
static uint8_t g_temp_pool[1024 * 1024]; // 1MB temp pool
static size_t g_pool_offset = 0;

void *nova_temp_pool_alloc(size_t size) {
  // Align offset to 16 bytes
  g_pool_offset = (g_pool_offset + 15) & ~15;

  if (g_pool_offset + size > sizeof(g_temp_pool)) {
    return NULL; // Pool exhausted
  }

  void *ptr = &g_temp_pool[g_pool_offset];
  g_pool_offset += size;
  return ptr;
}

void nova_temp_pool_reset(void) { g_pool_offset = 0; }
