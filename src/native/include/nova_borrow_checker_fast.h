/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA FAST BORROW CHECKER - Bitvector & Graph Based
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_BORROW_CHECKER_FAST_H
#define NOVA_BORROW_CHECKER_FAST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint64_t *immutable_borrows; // Bitvector - 64 vars per word
  uint64_t *mutable_borrows;   // Bitvector
  uint64_t *moved_vars;        // Bitvector
  size_t var_capacity;         // In number of vars
} FastBorrowTracker;

static inline FastBorrowTracker *fbt_create(size_t max_vars) {
  FastBorrowTracker *tracker = malloc(sizeof(FastBorrowTracker));
  size_t words = (max_vars + 63) / 64;
  tracker->immutable_borrows = calloc(words, sizeof(uint64_t));
  tracker->mutable_borrows = calloc(words, sizeof(uint64_t));
  tracker->moved_vars = calloc(words, sizeof(uint64_t));
  tracker->var_capacity = max_vars;
  return tracker;
}

static inline void fbt_destroy(FastBorrowTracker *tracker) {
  free(tracker->immutable_borrows);
  free(tracker->mutable_borrows);
  free(tracker->moved_vars);
  free(tracker);
}

// O(1) check!
static inline bool fbt_can_borrow_mutable(FastBorrowTracker *tracker,
                                          uint32_t var_id) {
  uint32_t word = var_id / 64;
  uint32_t bit = var_id % 64;
  uint64_t mask = 1ULL << bit;

  // Can borrow mutable if NO immutable and NO mutable borrows and NOT moved
  return !(tracker->immutable_borrows[word] & mask) &&
         !(tracker->mutable_borrows[word] & mask) &&
         !(tracker->moved_vars[word] & mask);
}

static inline bool fbt_can_borrow_immutable(FastBorrowTracker *tracker,
                                            uint32_t var_id) {
  uint32_t word = var_id / 64;
  uint32_t bit = var_id % 64;
  uint64_t mask = 1ULL << bit;

  // Can borrow immutable if NO mutable borrows and NOT moved
  return !(tracker->mutable_borrows[word] & mask) &&
         !(tracker->moved_vars[word] & mask);
}

static inline void fbt_add_mutable_borrow(FastBorrowTracker *tracker,
                                          uint32_t var_id) {
  tracker->mutable_borrows[var_id / 64] |= (1ULL << (var_id % 64));
}

static inline void fbt_add_immutable_borrow(FastBorrowTracker *tracker,
                                            uint32_t var_id) {
  tracker->immutable_borrows[var_id / 64] |= (1ULL << (var_id % 64));
}

static inline void fbt_mark_moved(FastBorrowTracker *tracker, uint32_t var_id) {
  tracker->moved_vars[var_id / 64] |= (1ULL << (var_id % 64));
}

#endif
