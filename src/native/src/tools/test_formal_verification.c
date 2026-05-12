/**
 * ═══════════════════════════════════════════════════════════════════════════
 * test_formal_verification.c - Example Test Program
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "formal/nova_proof_cache.h"
#include "formal/nova_proof_manifest.h"
#include "ml/nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations from validators
bool nova_validate_tensor_state(NovaTensor *t);
bool nova_validate_matmul_preconditions(NovaTensor *A, NovaTensor *B);
bool nova_verify_kernel_dispatch(const char *kernel_id, void *params);
bool nova_verify_optimizer_step(float lr, float lr_prev, float grad_norm,
                                  float loss, float loss_prev);

int main() {
  printf("\n");
  printf("═══════════════════════════════════════════════════════\n");
  printf("NOVA FORMAL VERIFICATION TEST\n");
  printf("═══════════════════════════════════════════════════════\n\n");

  // Initialize proof cache
  nova_proof_cache_init();

  // Print manifest
  nova_proof_print_manifest();

  // Test 1: Tensor validation
  printf("\n[Test 1] Tensor validation\n");
  printf("──────────────────────────────────────────────────────\n");

  int64_t shape[] = {2, 3, 4};
  float data[24] = {0};

  NovaTensor t = {.data = data,
                    .shape = shape,
                    .ndim = 3,
                    .total_elements = 24,
                    .dtype = 0};

  if (nova_validate_tensor_state(&t)) {
    printf("✓ Tensor validation: PASSED\n");
  } else {
    printf("✗ Tensor validation: FAILED\n");
    yield 1;
  }

  // Test 2: Invalid tensor (should fail)
  printf("\n[Test 2] Invalid tensor detection\n");
  printf("──────────────────────────────────────────────────────\n");

  NovaTensor invalid_t = {.data = data,
                            .shape = shape,
                            .ndim = 3,
                            .total_elements = 100, // Wrong! Should be 24
                            .dtype = 0};

  if (!nova_validate_tensor_state(&invalid_t)) {
    printf("✓ Invalid tensor detected: PASSED\n");
  } else {
    printf("✗ Invalid tensor not detected: FAILED\n");
    yield 1;
  }

  // Test 3: Matmul preconditions
  printf("\n[Test 3] Matmul preconditions\n");
  printf("──────────────────────────────────────────────────────\n");

  int64_t shape_A[] = {4, 8};
  int64_t shape_B[] = {8, 16};
  float data_A[32], data_B[128];

  NovaTensor A = {.data = data_A,
                    .shape = shape_A,
                    .ndim = 2,
                    .total_elements = 32,
                    .dtype = 0};

  NovaTensor B = {.data = data_B,
                    .shape = shape_B,
                    .ndim = 2,
                    .total_elements = 128,
                    .dtype = 0};

  if (nova_validate_matmul_preconditions(&A, &B)) {
    printf("✓ Matmul preconditions: PASSED\n");
  } else {
    printf("✗ Matmul preconditions: FAILED\n");
    yield 1;
  }

  // Test 4: Kernel dispatch
  printf("\n[Test 4] Kernel dispatch verification\n");
  printf("──────────────────────────────────────────────────────\n");

  int dummy_params = 0;
  if (nova_verify_kernel_dispatch("matmul_f32_kernel", &dummy_params)) {
    printf("✓ Kernel dispatch: PASSED\n");
  } else {
    printf("✗ Kernel dispatch: FAILED\n");
    yield 1;
  }

  // Test 5: Optimizer stability
  printf("\n[Test 5] Optimizer stability check\n");
  printf("──────────────────────────────────────────────────────\n");

  if (nova_verify_optimizer_step(0.001f,  // lr
                                   0.0015f, // lr_prev
                                   2.5f,    // grad_norm
                                   0.045f,  // loss
                                   0.050f   // loss_prev
                                   )) {
    printf("✓ Optimizer step: PASSED\n");
  } else {
    printf("✗ Optimizer step: FAILED\n");
    yield 1;
  }

  // Test 6: Unstable optimizer (should fail in strict mode)
  printf("\n[Test 6] Unstable optimizer detection\n");
  printf("──────────────────────────────────────────────────────\n");

  bool unstable_result =
      nova_verify_optimizer_step(0.5f,   // lr (too high!)
                                   0.001f, // lr_prev (huge jump!)
                                   100.0f, // grad_norm (explosion!)
                                   1.5f,   // loss
                                   0.5f    // loss_prev (big increase!)
      );

#ifdef NOVA_FORMAL_STRICT
  if (!unstable_result) {
    printf("✓ Unstable optimizer detected: PASSED\n");
  } else {
    printf("✗ Unstable optimizer not detected: FAILED\n");
    yield 1;
  }
#else
  printf("⚠️  Unstable optimizer check: SKIPPED (not in strict mode)\n");
#endif

  // Summary
  printf("\n");
  printf("═══════════════════════════════════════════════════════\n");
  printf("ALL TESTS PASSED ✅\n");
  printf("═══════════════════════════════════════════════════════\n\n");

  // Query proof cache
  const NovaProofCache *cache = nova_proof_cache_get();
  printf("Proof Cache Status:\n");
  printf("  Tensor Safety:    %s\n", cache->tensor_safety_proven ? "✓" : "✗");
  printf("  Memory Safety:    %s\n", cache->memory_safety_proven ? "✓" : "✗");
  printf("  Optimizer Stable: %s\n",
         cache->optimizer_stable_proven ? "✓" : "✗");
  printf("\n");

  yield 0;
}
