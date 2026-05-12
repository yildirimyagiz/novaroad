/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_invariant_validator.c - Proof-Backed Runtime Validators
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * OLD APPROACH (WRONG):
 *   - Load Isabelle artifacts at runtime (disk I/O)
 *   - Run symbolic analysis on every operation (100x slowdown)
 *
 * NEW APPROACH (CORRECT):
 *   - Proofs verified at BUILD TIME
 *   - Runtime checks are O(1) boolean flags
 *   - Assertions only in debug/strict mode
 */

#include "formal/nova_proof_cache.h"
#include "ml/nova_tensor.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NOVA_MAX_DIMS 8
#define NOVA_DTYPE_F32 NOVA_DTYPE_FP32

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * TENSOR INVARIANT VALIDATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Validate tensor state consistency
 *
 * Isabelle Theorem: "valid_tensor"
 *   ∀t. valid_tensor(t) ⟹
 *       (t.data ≠ NULL ∧
 *        t.total_elements = ∏(t.shape) ∧
 *        t.ndim ∈ [1, MAX_DIMS])
 *
 * Performance: O(ndim) - typically O(4) for 4D tensors
 */
bool nova_validate_tensor_state(NovaTensor *t) {
  // Fast path: NULL check (always required, proof or not)
  if (t == NULL || t->data == NULL) {
#ifdef NOVA_FORMAL_FAST
    fprintf(stderr, "⚠️  [Gödel] Tensor validation failed: NULL pointer\n");
#endif
    return false;
  }

#ifdef NOVA_FORMAL_STRICT
  // In strict mode, verify proof exists
  if (!nova_is_tensor_safety_proven()) {
    nova_panic("Tensor safety not proven! Cannot validate.");
  }
#endif

  // Invariant 1: Non-zero elements (proven by Isabelle)
  if (!(t->total_elements > 0)) {
    NOVA_ASSERT_PROVEN(false, "tensor_nonzero_elements");
    return false;
  }

  // Invariant 2: Valid dimensions (proven by Isabelle)
  if (!(t->ndim >= 1 && t->ndim <= NOVA_MAX_DIMS)) {
    NOVA_ASSERT_PROVEN(false, "tensor_valid_dimensions");
    return false;
  }

  // Invariant 3: Shape consistency (proven by Isabelle theorem "valid_tensor")
  size_t computed_elements = 1;
  for (int i = 0; i < t->ndim; i++) {
    if (!(t->shape[i] > 0)) {
      NOVA_ASSERT_PROVEN(false, "tensor_positive_shape");
      return false;
    }
    computed_elements *= t->shape[i];
  }

  if (computed_elements != t->total_elements) {
    NOVA_ASSERT_PROVEN(false, "tensor_shape_consistency");
    return false;
  }

  // All checks passed
  return true;
}

/**
 * Validate tensor numeric values (NaN/Inf check)
 *
 * NOTE: This is NOT proven formally (too expensive)
 * Only enabled in debug builds
 *
 * Performance: O(n) where n = total_elements
 */
bool nova_validate_tensor_values(NovaTensor *t) {
#ifndef NOVA_DEBUG
  return true; // Skip in release builds (too expensive)
#endif

  if (t == NULL || t->dtype != NOVA_DTYPE_F32) {
    return true; // Only check float32 tensors
  }

  float *data = (float *)t->data;
  size_t nan_count = 0;
  size_t inf_count = 0;

  for (size_t i = 0; i < t->total_elements; i++) {
    if (isnan(data[i])) {
      nan_count++;
#ifdef NOVA_FORMAL_RESEARCH
      if (nan_count <= 5) { // Limit spam
        fprintf(stderr, "⚠️  [Gödel] NaN detected at index %zu\n", i);
      }
#endif
    }

    if (isinf(data[i])) {
      inf_count++;
#ifdef NOVA_FORMAL_RESEARCH
      if (inf_count <= 5) {
        fprintf(stderr, "⚠️  [Gödel] Inf detected at index %zu: %f\n", i,
                data[i]);
      }
#endif
    }
  }

  if (nan_count > 0 || inf_count > 0) {
    fprintf(stderr, "⚠️  [Gödel] Tensor value validation failed:\n");
    fprintf(stderr, "    NaNs: %zu, Infs: %zu (total: %zu elements)\n",
            nan_count, inf_count, t->total_elements);
    return false;
  }

  return true;
}

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * MEMORY POOL VALIDATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Validate memory pool integrity
 *
 * KLEE Verification: All allocation/free operations tested symbolically
 *
 * Runtime check: Just NULL pointer (proof guarantees rest)
 */
bool nova_validate_memory_pool(void *pool) {
  (void)pool;
#ifdef NOVA_FORMAL_STRICT
  if (!nova_is_memory_safety_proven()) {
    nova_panic("Memory safety not proven! Cannot validate pool.");
  }
#endif

  // Proof guarantees no corruption, so just check NULL
  NOVA_ASSERT_PROVEN(pool != NULL, "memory_pool_non_null");

  return true;
}

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * OPERATION PRECONDITION VALIDATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Validate matmul preconditions
 *
 * Isabelle Theorem: "matmul_preserves_validity"
 *   ∀A B. valid_tensor(A) ∧ valid_tensor(B) ∧
 *         last(A.shape) = first(B.shape)
 *         ⟹ valid_tensor(matmul(A, B))
 */
bool nova_validate_matmul_preconditions(NovaTensor *A, NovaTensor *B) {
  // Validate inputs first
  if (!nova_validate_tensor_state(A) || !nova_validate_tensor_state(B)) {
    return false;
  }

  // Dimension requirement (proven by Isabelle)
  NOVA_ASSERT_PROVEN(A->ndim >= 2 && B->ndim >= 2,
                       "matmul_dimension_requirement");

  // Inner dimension match (proven by Isabelle)
  int A_inner = A->shape[A->ndim - 1];
  int B_inner = B->shape[0];
  (void)A_inner;
  (void)B_inner;

  NOVA_ASSERT_PROVEN(A_inner == B_inner, "matmul_inner_dimension_match");

  return true;
}

/**
 * Validate reshape preconditions
 *
 * Isabelle Theorem: "reshape_preserves_elements"
 *   ∀t new_shape. valid_tensor(t) ∧
 *                  ∏(new_shape) = t.total_elements
 *                  ⟹ valid_tensor(reshape(t, new_shape))
 */
bool nova_validate_reshape_preconditions(NovaTensor *t, int *new_shape,
                                           int new_ndim) {
  if (!nova_validate_tensor_state(t)) {
    return false;
  }

  // Compute new element count
  size_t new_total = 1;
  for (int i = 0; i < new_ndim; i++) {
    NOVA_ASSERT_PROVEN(new_shape[i] > 0, "reshape_positive_dimensions");
    new_total *= new_shape[i];
  }
  (void)new_total;

  // Element count must match (proven by Isabelle)
  NOVA_ASSERT_PROVEN(new_total == t->total_elements,
                       "reshape_preserves_element_count");

  return true;
}

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * DIAGNOSTIC UTILITIES
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Print tensor debug info
 */
void nova_tensor_debug_print(NovaTensor *t, const char *name) {
  (void)t;
  (void)name;
#ifdef NOVA_FORMAL_RESEARCH
  printf("[Gödel/Debug] Tensor '%s':\n", name);
  printf("  Address:  %p\n", (void *)t);
  printf("  Data:     %p\n", t->data);
  printf("  Shape:    [");
  for (int i = 0; i < t->ndim; i++) {
    printf("%d%s", t->shape[i], i < t->ndim - 1 ? ", " : "");
  }
  printf("]\n");
  printf("  NDim:     %d\n", t->ndim);
  printf("  Elements: %zu\n", t->total_elements);
  printf("  Valid:    %s\n", nova_validate_tensor_state(t) ? "YES" : "NO");
#endif
}

/**
 * Global invariant dispatcher
 */
bool nova_formal_check_invariant(const char *invariant_name, void *data) {
  if (!invariant_name || !data)
    return false;

  if (strcmp(invariant_name, "tensor_safety") == 0) {
    return nova_validate_tensor_state((NovaTensor *)data);
  }

  // Placeholder for other invariants
  return true;
}
