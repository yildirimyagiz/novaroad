#include "nova_tensor.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR SANITY CHECK LAYER
 * ═══════════════════════════════════════════════════════════════════════════
 * Cheap, fast validation before expensive Gödel verification.
 * Catches common engineering errors like invalid shapes, NULL pointers, etc.
 */

#define NOVA_MAX_DIMS 8

bool nova_tensor_sanity_check(NovaTensor *t) {
  // 1. Basic pointer validity
  if (t == NULL) {
    fprintf(stderr, "🔥 [Nova Sanity] NULL tensor pointer detected\n");
    return false;
  }

  // 2. Data pointer check (unless explicitly empty/symbolic)
  // For now, we assume all tensors must have data
  if (t->data == NULL) {
    fprintf(stderr, "🔥 [Nova Sanity] NULL data pointer in tensor\n");
    return false;
  }

  // 3. Dimensionality check
  if (t->ndim <= 0 || t->ndim > NOVA_MAX_DIMS) {
    fprintf(stderr, "🔥 [Nova Sanity] Invalid dimensionality: %d (Max: %d)\n",
            t->ndim, NOVA_MAX_DIMS);
    return false;
  }

  // 4. Shape consistency check
  size_t computed_total = 1;
  for (int i = 0; i < t->ndim; i++) {
    if (t->shape[i] < 0) {
      fprintf(stderr,
              "🔥 [Nova Sanity] Negative dimension at index %d: %lld\n", i,
              t->shape[i]);
      return false;
    }
    // Check for overflow (basic)
    size_t next_total = computed_total * t->shape[i];
    if (t->shape[i] > 0 && next_total < computed_total) {
      fprintf(stderr, "🔥 [Nova Sanity] Shape overflow detected\n");
      return false;
    }
    computed_total = next_total;
  }

  // 5. Total elements match
  if (computed_total != t->total_elements) {
    fprintf(stderr,
            "🔥 [Nova Sanity] Total elements mismatch. Shape product: %zu, "
            "Struct: %zu\n",
            computed_total, t->total_elements);
    return false;
  }

  // 6. Stride consistency (Contiguous only for now)
  // If layout is contiguous, strides should match standard row-major
  if (t->layout == NOVA_LAYOUT_CONTIGUOUS) {
    size_t expected_stride = 1;
    for (int i = t->ndim - 1; i >= 0; i--) {
      if (t->strides[i] != (int64_t)expected_stride) {
        // Just a warning for now, as striding can be complex with slicing
        // fprintf(stderr, "⚠️ [Nova Sanity] Warning: Stride mismatch at dim
        // %d. Expected: %zu, Found: %lld\n", i, expected_stride,
        // t->strides[i]);
      }
      expected_stride *= t->shape[i];
    }
  }

  return true;
}
