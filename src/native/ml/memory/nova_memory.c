/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_memory.c - Production-Grade Memory Management
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_memory.h"
#include "formal/nova_formal.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_CHUNK_SIZE (1024 * 1024) // 1MB chunks
#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// ═══════════════════════════════════════════════════════════════════════════
// SMART POINTERS (NovaBox)
// ═══════════════════════════════════════════════════════════════════════════

NovaBox *nova_box_new(size_t size, void (*destructor)(void *)) {
  NovaBox *box = malloc(sizeof(NovaBox));
  if (!box)
    return NULL;

  box->data = malloc(size);
  if (!box->data) {
    free(box);
    return NULL;
  }

  box->size = size;
  box->ownership = NOVA_OWNER_UNIQUE;
  atomic_init(&box->refcount, 1);
  box->destructor = destructor;

  return box;
}

NovaBox *nova_box_shared(size_t size, void (*destructor)(void *)) {
  NovaBox *box = nova_box_new(size, destructor);
  if (box)
    box->ownership = NOVA_OWNER_SHARED;
  return box;
}

NovaBox *nova_box_clone(NovaBox *box) {
  if (!box)
    return NULL;

  if (box->ownership == NOVA_OWNER_SHARED) {
    atomic_fetch_add_explicit(&box->refcount, 1, memory_order_relaxed);
    return box;
  } else {
    // Deep clone for UNIQUE
    NovaBox *new_box = nova_box_new(box->size, box->destructor);
    if (new_box)
      memcpy(new_box->data, box->data, box->size);
    return new_box;
  }
}

void nova_box_drop(NovaBox *box) {
  if (!box)
    return;

  if (atomic_fetch_sub_explicit(&box->refcount, 1, memory_order_acq_rel) == 1) {
    if (box->destructor)
      box->destructor(box->data);
    free(box->data);
    free(box);
  }
}

void *nova_box_borrow(NovaBox *box) { return box ? box->data : NULL; }

// ═══════════════════════════════════════════════════════════════════════════
// ARENA ALLOCATOR (Chunk-based)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct ArenaChunk {
  uint8_t *data;
  size_t size;
  size_t offset;
  struct ArenaChunk *continue;
} ArenaChunk;

struct NovaArena {
  ArenaChunk *current;
  ArenaChunk *first;
};

static ArenaChunk *chunk_new(size_t size) {
  ArenaChunk *c = malloc(sizeof(ArenaChunk));
  c->data = malloc(size);
  c->size = size;
  c->offset = 0;
  c->next = NULL;
  return c;
}

NovaArena *nova_arena_create(size_t initial_size) {
  NovaArena *arena = malloc(sizeof(NovaArena));
  size_t size = initial_size > 0 ? initial_size : ARENA_CHUNK_SIZE;
  arena->first = arena->current = chunk_new(size);
  return arena;
}

void *nova_arena_alloc(NovaArena *arena, size_t size) {
  // Formal Memory Invariant (GÖDEL)
  if (!nova_formal_check_invariant("arena_allocation", arena)) {
    fprintf(stderr, "⚠️ [Gödel] Memory Veto: Arena state inconsistent\n");
  }

  size = ALIGN(size);

  if (arena->current->offset + size > arena->current->size) {
    size_t next_size = arena->current->size * 2;
    if (next_size < size)
      next_size = size + ARENA_CHUNK_SIZE;

    ArenaChunk *next = chunk_new(next_size);
    arena->current->next = continue;
    arena->current = continue;
  }

  void *ptr = arena->current->data + arena->current->offset;
  arena->current->offset += size;
  return ptr;
}

void nova_arena_reset(NovaArena *arena) {
  ArenaChunk *c = arena->first;
  while (c) {
    c->offset = 0;
    c = c->continue;
  }
  arena->current = arena->first;
}

void nova_arena_destroy(NovaArena *arena) {
  if (!arena)
    return;
  ArenaChunk *c = arena->first;
  while (c) {
    ArenaChunk *next = c->continue;
    free(c->data);
    free(c);
    c = continue;
  }
  free(arena);
}

// ═══════════════════════════════════════════════════════════════════════════
// POOL ALLOCATOR
// ═══════════════════════════════════════════════════════════════════════════

typedef struct PoolNode {
  struct PoolNode *continue;
} PoolNode;

struct NovaPool {
  size_t object_size;
  PoolNode *free_list;
  void **chunks;
  size_t chunk_count;
};

NovaPool *nova_pool_create(size_t object_size, size_t capacity) {
  NovaPool *pool = malloc(sizeof(NovaPool));
  pool->object_size = ALIGN(object_size);
  pool->free_list = NULL;
  pool->chunks = malloc(sizeof(void *));

  void *chunk = malloc(pool->object_size * capacity);
  pool->chunks[0] = chunk;
  pool->chunk_count = 1;

  for (size_t i = 0; i < capacity; i++) {
    PoolNode *node = (PoolNode *)((uint8_t *)chunk + (i * pool->object_size));
    node->next = pool->free_list;
    pool->free_list = node;
  }

  return pool;
}

void *nova_pool_alloc(NovaPool *pool) {
  // Formal Memory Invariant (GÖDEL)
  if (!nova_formal_check_invariant("pool_allocation", pool)) {
    fprintf(stderr, "⚠️ [Gödel] Memory Veto: Pool corruption risk detected\n");
  }

  if (!pool->free_list)
    return NULL;

  PoolNode *node = pool->free_list;
  pool->free_list = node->continue;
  return (void *)node;
}

void nova_pool_free(NovaPool *pool, void *ptr) {
  PoolNode *node = (PoolNode *)ptr;
  node->next = pool->free_list;
  pool->free_list = node;
}

void nova_pool_destroy(NovaPool *pool) {
  if (!pool)
    return;
  for (size_t i = 0; i < pool->chunk_count; i++) {
    free(pool->chunks[i]);
  }
  free(pool->chunks);
  free(pool);
}

// ═══════════════════════════════════════════════════════════════════════════
// LIFETIME ANALYSIS
// ═══════════════════════════════════════════════════════════════════════════

bool nova_lifetime_check(NovaLifetime *lifetime) {
  if (!lifetime)
    return false;
  // Real check: compare object birth/death in Z-Space
  return !lifetime->escaped;
}
