/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ARENA ALLOCATOR - Fast Memory Management
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_ARENA_H
#define NOVA_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_DEFAULT_CAPACITY (1024 * 1024) // 1MB default
#define ARENA_ALIGNMENT 8                    // 8-byte alignment
#define ARENA_LARGE_THRESHOLD (64 * 1024)    // 64KB threshold

typedef struct ArenaBlock {
  uint8_t *buffer;
  size_t capacity;
  size_t offset;
  struct ArenaBlock *next;
} ArenaBlock;

typedef struct Arena {
  ArenaBlock *current;
  ArenaBlock *blocks;
  size_t total_allocated;
  size_t total_requested;
  size_t block_count;
  size_t allocation_count;
  size_t large_allocation_count;
} Arena;

static inline ArenaBlock *arena_block_create(size_t capacity) {
  ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
  if (!block)
    return NULL;
  block->buffer = (uint8_t *)malloc(capacity);
  if (!block->buffer) {
    free(block);
    return NULL;
  }
  block->capacity = capacity;
  block->offset = 0;
  block->next = NULL;
  return block;
}

static inline Arena *arena_create(size_t initial_capacity) {
  if (initial_capacity == 0)
    initial_capacity = ARENA_DEFAULT_CAPACITY;
  Arena *arena = (Arena *)malloc(sizeof(Arena));
  if (!arena)
    return NULL;
  arena->current = arena_block_create(initial_capacity);
  if (!arena->current) {
    free(arena);
    return NULL;
  }
  arena->blocks = arena->current;
  arena->total_allocated = initial_capacity;
  arena->total_requested = 0;
  arena->block_count = 1;
  arena->allocation_count = 0;
  arena->large_allocation_count = 0;
  return arena;
}

static inline void arena_destroy(Arena *arena) {
  if (!arena)
    return;
  ArenaBlock *block = arena->blocks;
  while (block) {
    ArenaBlock *next = block->next;
    free(block->buffer);
    free(block);
    block = next;
  }
  free(arena);
}

static inline void *arena_alloc(Arena *arena, size_t size) {
  size_t aligned_size = (size + ARENA_ALIGNMENT - 1) & ~(ARENA_ALIGNMENT - 1);

  // Fast path
  if (arena->current->offset + aligned_size <= arena->current->capacity) {
    void *ptr = arena->current->buffer + arena->current->offset;
    arena->current->offset += aligned_size;
    arena->total_requested += size;
    arena->allocation_count++;
    return ptr;
  }

  // Slow path
  if (aligned_size > ARENA_LARGE_THRESHOLD) {
    ArenaBlock *large_block = arena_block_create(aligned_size);
    if (!large_block)
      return NULL;
    large_block->next = arena->blocks;
    arena->blocks = large_block;
    large_block->offset = aligned_size;
    arena->total_allocated += aligned_size;
    arena->total_requested += size;
    arena->block_count++;
    arena->allocation_count++;
    arena->large_allocation_count++;
    return large_block->buffer;
  }

  size_t new_capacity = arena->current->capacity * 2;
  if (new_capacity < aligned_size)
    new_capacity = aligned_size * 2;
  ArenaBlock *new_block = arena_block_create(new_capacity);
  if (!new_block)
    return NULL;
  new_block->next = arena->blocks;
  arena->blocks = new_block;
  arena->current = new_block;
  void *ptr = new_block->buffer;
  new_block->offset = aligned_size;
  arena->total_allocated += new_capacity;
  arena->total_requested += size;
  arena->block_count++;
  arena->allocation_count++;
  return ptr;
}

static inline char *arena_strdup(Arena *arena, const char *str) {
  if (!str)
    return NULL;
  size_t len = strlen(str);
  char *copy = (char *)arena_alloc(arena, len + 1);
  memcpy(copy, str, len + 1);
  return copy;
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARKING
// ═══════════════════════════════════════════════════════════════════════════

#include <time.h>

static inline void benchmark_arena_performance(void) {
  const size_t NUM_ALLOCATIONS = 100000;
  const size_t ALLOC_SIZE = 64;

  printf("\n🚀 NOVA PERFORMANCE BENCHMARK: Arena vs Malloc\n");
  printf("Allocations: %zu | Size: %zu bytes\n", NUM_ALLOCATIONS, ALLOC_SIZE);

  // Malloc
  clock_t start = clock();
  void **ptrs = malloc(NUM_ALLOCATIONS * sizeof(void *));
  for (size_t i = 0; i < NUM_ALLOCATIONS; i++)
    ptrs[i] = malloc(ALLOC_SIZE);
  for (size_t i = 0; i < NUM_ALLOCATIONS; i++)
    free(ptrs[i]);
  free(ptrs);
  clock_t end = clock();
  double malloc_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

  // Arena
  start = clock();
  Arena *arena = arena_create(1024 * 1024);
  for (size_t i = 0; i < NUM_ALLOCATIONS; i++)
    arena_alloc(arena, ALLOC_SIZE);
  arena_destroy(arena);
  end = clock();
  double arena_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

  printf("Standard Malloc: %.2f ms\n", malloc_time);
  printf("Nova Arena:    %.2f ms\n", arena_time);
  printf("⚡ Improvement:   %.2fx faster\n\n", malloc_time / arena_time);
}

#endif
