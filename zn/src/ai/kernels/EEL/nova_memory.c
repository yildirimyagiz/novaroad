/*
 * nova_memory.c — Memory Planner / Traffic Optimizer
 *
 * - Arena allocator (O(1) alloc, bulk-free)
 * - Tensor lifetime tracking
 * - Buffer reuse (alias tensors that don't overlap in lifetime)
 * - Cache-friendly layout (row-major, aligned to 64 bytes)
 * - VRAM/RAM pressure reduction via greedy interval coloring
 */

#include "nova.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Arena allocator
 * ========================================================================= */

NovaArena nova_arena_create(size_t capacity) {
  NovaArena a;
  memset(&a, 0, sizeof(a));
  a.base = (uint8_t *)malloc(capacity);
  a.capacity = a.base ? capacity : 0;
  return a;
}

void nova_arena_destroy(NovaArena *a) {
  if (!a)
    return;
  free(a->base);
  a->base = NULL;
  a->capacity = 0;
  a->used = 0;
}

void *nova_arena_alloc(NovaArena *a, size_t bytes, size_t align) {
  if (!a || !a->base || bytes == 0)
    return NULL;
  if (align == 0)
    align = NOVA_ARENA_ALIGNMENT;

  /* Align current cursor */
  size_t cur = a->used;
  size_t pad = (align - (cur % align)) % align;
  size_t new_used = cur + pad + bytes;

  if (new_used > a->capacity)
    return NULL; /* OOM */

  void *ptr = a->base + cur + pad;
  a->used = new_used;
  a->num_allocs++;

  if (a->used > a->peak)
    a->peak = a->used;

  /* Zero-initialize to ensure determinism */
  memset(ptr, 0, bytes);
  return ptr;
}

void nova_arena_reset(NovaArena *a) {
  if (!a)
    return;
  a->used = 0;
  /* Don't reset peak — keep it for stats */
}

/* =========================================================================
 * Buffer reuse via interval graph coloring (greedy)
 *
 * Algorithm:
 *  1. Sort tensors by first_use.
 *  2. Maintain a list of "free slots" (previously freed tensor buffers).
 *  3. For each tensor T in order: find a free slot whose size >= T->nbytes
 *     and whose last-user's last_use < T->first_use. If found, alias T
 *     to that slot (saving allocation). Otherwise, allocate fresh.
 *
 * This is a greedy first-fit interval coloring. Optimal is NP-hard
 * but greedy achieves good results for typical DL graphs.
 * ========================================================================= */

typedef struct {
  uint32_t tensor_id; /* tensor that owns the buffer */
  size_t nbytes;
  int32_t last_use; /* when this buffer becomes free again */
} FreeSlot;

NovaStatus nova_memory_plan(NovaGraph *g) {
  if (!g)
    return NOVA_ERR_NULL_PTR;
  if (!g->optimized)
    fprintf(stderr, "[nova] WARNING: memory_plan called before optimize\n");

  uint32_t nt = g->num_tensors;
  uint32_t *order = malloc(nt * sizeof(uint32_t));
  FreeSlot *slots = malloc(nt * sizeof(FreeSlot));
  if (!order || !slots) {
    free(order);
    free(slots);
    return NOVA_ERR_OOM;
  }

  /* Build sorted order by first_use */
  for (uint32_t i = 0; i < nt; ++i)
    order[i] = i;
  /* Insertion sort (small n) */
  for (uint32_t i = 1; i < nt; ++i) {
    uint32_t key = order[i];
    int32_t key_fu = g->tensors[key].first_use;
    int j = (int)i - 1;
    while (j >= 0 && g->tensors[order[j]].first_use > key_fu) {
      order[j + 1] = order[j];
      j--;
    }
    order[j + 1] = key;
  }

  uint32_t num_slots = 0;
  size_t total_allocated = 0;
  size_t total_reused = 0;

  for (uint32_t oi = 0; oi < nt; ++oi) {
    NovaTensor *t = &g->tensors[order[oi]];

    /* Parameters and graph inputs are never reused */
    if (t->is_parameter || t->first_use == INT32_MAX)
      continue;

    /* Try to find a reusable slot */
    bool found = false;
    for (uint32_t s = 0; s < num_slots; ++s) {
      FreeSlot *sl = &slots[s];
      /* Slot is free when sl->last_use < t->first_use */
      if (sl->last_use < t->first_use && sl->nbytes >= t->nbytes) {
        /* Alias t → sl->tensor_id */
        t->is_alias = true;
        t->alias_of = sl->tensor_id;
        total_reused += t->nbytes;

        /* Update slot's last_use */
        if (t->last_use > sl->last_use)
          sl->last_use = t->last_use;
        found = true;
        break;
      }
    }

    if (!found && num_slots < nt) {
      /* New slot */
      slots[num_slots].tensor_id = t->id;
      slots[num_slots].nbytes = t->nbytes;
      slots[num_slots].last_use = t->last_use;
      num_slots++;
      total_allocated += t->nbytes;
    }
  }

  fprintf(stderr,
          "[nova] memory_plan: allocated=%.2f MB reused=%.2f MB saved=%.1f%%\n",
          (double)total_allocated / (1 << 20), (double)total_reused / (1 << 20),
          total_allocated + total_reused > 0
              ? 100.0 * total_reused / (total_allocated + total_reused)
              : 0.0);

  g->memory_planned = true;
  free(order);
  free(slots);
  return NOVA_OK;
}

/* =========================================================================
 * Allocate physical memory for all tensors
 * ========================================================================= */

NovaStatus nova_memory_allocate(NovaGraph *g) {
  if (!g)
    return NOVA_ERR_NULL_PTR;
  if (!g->memory_planned) {
    NovaStatus s = nova_memory_plan(g);
    if (s != NOVA_OK)
      return s;
  }

  NovaBackend *b = g->backend;

  for (uint32_t i = 0; i < g->num_tensors; ++i) {
    NovaTensor *t = &g->tensors[i];
    if (t->data)
      continue; /* already allocated */

    if (t->is_alias) {
      /* Point to aliased tensor's buffer */
      NovaTensor *src = &g->tensors[t->alias_of];
      if (!src->data) {
        /* src not yet allocated — allocate it first */
        if (b && b->alloc) {
          src->data = b->alloc(b, src->nbytes, NOVA_ARENA_ALIGNMENT);
        } else {
          src->data = nova_arena_alloc(src->is_parameter ? &g->param_arena
                                                           : &g->workspace,
                                         src->nbytes, NOVA_ARENA_ALIGNMENT);
        }
        if (!src->data)
          return NOVA_ERR_OOM;
      }
      t->data = src->data;
      continue;
    }

    if (b && b->alloc) {
      t->data = b->alloc(b, t->nbytes, NOVA_ARENA_ALIGNMENT);
    } else {
      NovaArena *arena = t->is_parameter ? &g->param_arena : &g->workspace;
      t->data = nova_arena_alloc(arena, t->nbytes, NOVA_ARENA_ALIGNMENT);
    }

    if (!t->data)
      return NOVA_ERR_OOM;
  }

  return NOVA_OK;
}
