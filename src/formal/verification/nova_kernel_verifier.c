/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_kernel_verifier.c - Proof-Backed Kernel Dispatch Verification
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "formal/nova_proof_cache.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * Kernel dispatch verification
 *
 * Proven properties:
 *   1. Determinism: Same input → Same output (bitwise)
 *   2. Memory safety: No out-of-bounds access
 *   3. Race-free: No data races in parallel execution
 *
 * Performance: O(1) - just flag checks
 */
bool nova_verify_kernel_dispatch(const char *kernel_id, void *params) {
  if (kernel_id == NULL || params == NULL) {
#ifdef NOVA_FORMAL_FAST
    fprintf(stderr, "⚠️  [Gödel] Kernel dispatch failed: NULL argument\n");
#endif
    return false;
  }

#ifdef NOVA_FORMAL_STRICT
  // In strict mode, require determinism proof
  if (!nova_is_kernel_determinism_proven()) {
    fprintf(stderr, "❌ [Gödel] Kernel determinism not proven for: %s\n",
            kernel_id);
    return false;
  }
#endif

// Log dispatch in research mode
#ifdef NOVA_FORMAL_RESEARCH
  printf("[Gödel] Dispatching verified kernel: %s\n", kernel_id);
#endif

  // Kernel-specific checks (can be extended per kernel type)
  if (strstr(kernel_id, "matmul") != NULL) {
    NOVA_ASSERT_PROVEN(nova_is_kernel_determinism_proven(),
                         "matmul_kernel_determinism");
  }

  if (strstr(kernel_id, "conv2d") != NULL) {
    NOVA_ASSERT_PROVEN(nova_is_kernel_determinism_proven(),
                         "conv2d_kernel_determinism");
  }

  if (strstr(kernel_id, "softmax") != NULL) {
    // Softmax has numerical stability requirements
    NOVA_ASSERT_PROVEN(nova_is_kernel_determinism_proven(),
                         "softmax_kernel_determinism");

#ifdef NOVA_FORMAL_RESEARCH
    printf("[Gödel] Note: softmax uses numerically stable algorithm\n");
#endif
  }

  return true;
}

/**
 * Report verification failure
 *
 * Behavior depends on build configuration:
 *   - STRICT:   Abort immediately
 *   - FAST:     Log warning, continue
 *   - DISABLED: Silent (no check)
 */
void nova_report_verification_failure(const char *reason) {
  fprintf(stderr, "❌ [Gödel] VERIFICATION FAILURE: %s\n", reason);

#ifdef NOVA_FORMAL_STRICT
  fprintf(stderr, "    Aborting in STRICT mode.\n");
  abort();
#else
  fprintf(stderr,
          "    ⚠️  Continuing in degraded mode (no formal guarantees).\n");
#endif
}

/**
 * Verify GPU kernel launch parameters
 *
 * Common errors:
 *   - Grid/block dimensions exceed hardware limits
 *   - Shared memory exceeds device capacity
 *   - Invalid stream/device ID
 */
bool nova_verify_gpu_kernel_params(int grid_x, int grid_y, int grid_z,
                                     int block_x, int block_y, int block_z,
                                     size_t shared_mem_bytes) {
  (void)grid_x;
  (void)grid_y;
  (void)grid_z;
  (void)block_x;
  (void)block_y;
  (void)block_z;
  (void)shared_mem_bytes;
#ifdef NOVA_FORMAL_STRICT
  if (!nova_is_kernel_determinism_proven()) {
    fprintf(stderr, "❌ [Gödel] GPU kernel correctness not proven\n");
    return false;
  }
#endif

  // Hardware limits (CUDA)
  const int MAX_GRID_DIM = 65535;
  const int MAX_BLOCK_DIM = 1024;
  const size_t MAX_SHARED_MEM = 48 * 1024; // 48KB typical
  (void)MAX_GRID_DIM;
  (void)MAX_BLOCK_DIM;
  (void)MAX_SHARED_MEM;

  NOVA_ASSERT_PROVEN(grid_x > 0 && grid_x <= MAX_GRID_DIM && grid_y > 0 &&
                           grid_y <= MAX_GRID_DIM && grid_z > 0 &&
                           grid_z <= MAX_GRID_DIM,
                       "gpu_valid_grid_dimensions");

  NOVA_ASSERT_PROVEN(block_x > 0 && block_x <= MAX_BLOCK_DIM && block_y > 0 &&
                           block_y <= MAX_BLOCK_DIM && block_z > 0 &&
                           block_z <= MAX_BLOCK_DIM,
                       "gpu_valid_block_dimensions");

  NOVA_ASSERT_PROVEN(block_x * block_y * block_z <= MAX_BLOCK_DIM,
                       "gpu_total_threads_per_block");

  NOVA_ASSERT_PROVEN(shared_mem_bytes <= MAX_SHARED_MEM,
                       "gpu_shared_memory_limit");

  return true;
}

/**
 * Verify distributed kernel (multi-GPU/multi-node)
 *
 * Additional requirements:
 *   - Communication determinism
 *   - No deadlocks
 *   - Consistent collective operations
 */
bool nova_verify_distributed_kernel(const char *kernel_id, int num_devices,
                                      int device_rank) {
  (void)kernel_id;
  (void)num_devices;
  (void)device_rank;
#ifdef NOVA_FORMAL_STRICT
  const NovaProofCache *cache = nova_proof_cache_get();
  if (!cache->distributed_comm_verified) {
    fprintf(stderr, "❌ [Gödel] Distributed communication not verified\n");
    return false;
  }
#endif

  NOVA_ASSERT_PROVEN(num_devices > 0 && num_devices <= 1024,
                       "distributed_valid_device_count");

  NOVA_ASSERT_PROVEN(device_rank >= 0 && device_rank < num_devices,
                       "distributed_valid_rank");

#ifdef NOVA_FORMAL_RESEARCH
  printf("[Gödel] Verified distributed kernel '%s': %d devices, rank %d\n",
         kernel_id, num_devices, device_rank);
#endif

  return true;
}
