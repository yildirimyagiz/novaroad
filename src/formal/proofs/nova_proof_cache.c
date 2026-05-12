/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_proof_cache.c - Runtime Proof Cache Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "formal/nova_proof_cache.h"
#include "formal/nova_proof_manifest.h"
#include <stdio.h>
#include <string.h>

/**
 * Global proof cache (initialized once at startup)
 */
NovaProofCache __nova_global_proof_cache = {0};
static bool __nova_cache_initialized = false;

/**
 * Initialize proof cache from manifest
 *
 * Performance: O(n) where n = number of proofs (~10)
 * Called ONCE at program startup
 */
void nova_proof_cache_init(void) {
  if (__nova_cache_initialized) {
    return; // Already initialized
  }

  printf("[Gödel] Initializing formal verification cache...\n");

  // Validate manifest integrity
  if (!nova_proof_validate_manifest()) {
    fprintf(stderr, "❌ [Gödel] CRITICAL: Proof manifest validation failed!\n");
    fprintf(stderr, "    Binary may not be formally verified.\n");
    fprintf(stderr, "    Build info: %s @ %s\n",
            NOVA_PROOF_MANIFEST.build_git_commit,
            NOVA_PROOF_MANIFEST.build_timestamp);

#ifdef NOVA_FORMAL_STRICT
    fprintf(stderr, "    Aborting in STRICT mode.\n");
    abort();
#else
    fprintf(stderr,
            "    ⚠️  Continuing in DEGRADED mode (no formal guarantees).\n");
#endif
  }

  // Populate cache from manifest (O(1) per proof)
  __nova_global_proof_cache.tensor_safety_proven =
      nova_proof_check("tensor_invariants");

  __nova_global_proof_cache.memory_safety_proven =
      nova_proof_check("memory_safety");

  __nova_global_proof_cache.optimizer_stable_proven =
      nova_proof_check("optimizer_stability");

  __nova_global_proof_cache.kernel_determinism_proven =
      nova_proof_check("kernel_determinism");

  __nova_global_proof_cache.autodiff_correctness_proven =
      nova_proof_check("autodiff_correctness");

  __nova_global_proof_cache.allocation_safety_proven =
      nova_proof_check("allocation_safety");

  __nova_global_proof_cache.convergence_guaranteed_proven =
      nova_proof_check("convergence_guarantee");

  __nova_global_proof_cache.gpu_kernel_verified =
      nova_proof_check("gpu_kernel_correctness");

  __nova_global_proof_cache.distributed_comm_verified =
      nova_proof_check("distributed_communication");

  // Print status
  printf("[Gödel] Proof cache initialized:\n");
  printf("  ✓ Tensor Safety:       %s\n",
         __nova_global_proof_cache.tensor_safety_proven ? "PROVEN"
                                                          : "UNVERIFIED");
  printf("  ✓ Memory Safety:       %s\n",
         __nova_global_proof_cache.memory_safety_proven ? "PROVEN"
                                                          : "UNVERIFIED");
  printf("  ✓ Optimizer Stable:    %s\n",
         __nova_global_proof_cache.optimizer_stable_proven ? "PROVEN"
                                                             : "UNVERIFIED");
  printf("  ✓ Kernel Determinism:  %s\n",
         __nova_global_proof_cache.kernel_determinism_proven ? "PROVEN"
                                                               : "UNVERIFIED");
  printf("  ✓ Autodiff Correct:    %s\n",
         __nova_global_proof_cache.autodiff_correctness_proven
             ? "PROVEN"
             : "UNVERIFIED");
  printf("  ✓ Allocation Safety:   %s\n",
         __nova_global_proof_cache.allocation_safety_proven ? "PROVEN"
                                                              : "UNVERIFIED");
  printf("  ✓ Convergence:         %s\n",
         __nova_global_proof_cache.convergence_guaranteed_proven
             ? "PROVEN"
             : "UNVERIFIED");
  printf("  ✓ GPU Kernels:         %s\n",
         __nova_global_proof_cache.gpu_kernel_verified ? "VERIFIED"
                                                         : "UNVERIFIED");
  printf("  ✓ Distributed Comm:    %s\n",
         __nova_global_proof_cache.distributed_comm_verified ? "VERIFIED"
                                                               : "UNVERIFIED");

  // Count verified proofs
  int verified_count = 0;
  if (__nova_global_proof_cache.tensor_safety_proven)
    verified_count++;
  if (__nova_global_proof_cache.memory_safety_proven)
    verified_count++;
  if (__nova_global_proof_cache.optimizer_stable_proven)
    verified_count++;
  if (__nova_global_proof_cache.kernel_determinism_proven)
    verified_count++;
  if (__nova_global_proof_cache.autodiff_correctness_proven)
    verified_count++;

  printf("[Gödel] %d/%d critical proofs verified.\n", verified_count, 5);

  if (verified_count < 5) {
    fprintf(stderr, "⚠️  [Gödel] Warning: Not all critical proofs verified!\n");
#ifdef NOVA_FORMAL_STRICT
    abort();
#endif
  }

  __nova_cache_initialized = true;
}

/**
 * Get cache pointer (for external queries)
 */
const NovaProofCache *nova_proof_cache_get(void) {
  if (!__nova_cache_initialized) {
    fprintf(stderr, "⚠️  [Gödel] Warning: Proof cache not initialized!\n");
    fprintf(stderr, "    Call nova_proof_cache_init() first.\n");
  }
  return &__nova_global_proof_cache;
}
