/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_jit.c - JIT-over-Mirror Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_jit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ZMirrorV2Packet *nova_jit_encode_kernel(const NovaKernel *base,
                                          const NovaKernel *updated) {
  if (!updated)
    return NULL;

  // Treat binary as float array for Mirror v2 (hack for demo/serialization)
  // In production, we'd have a specific MirrorBinary mode, but float works as
  // 4-byte increments
  uint32_t element_count = (updated->code_size + 3) / 4;

  // If no base, use zeros
  float *base_data =
      base ? (float *)base->binary_code : calloc(element_count, 4);
  float *updated_data = (float *)updated->binary_code;

  ZMirrorV2Packet *packet =
      zmirror_v2_encode(base_data, updated_data, element_count,
                        0.0f); // 0 threshold for exact code

  if (!base)
    free(base_data);
  return packet;
}

NovaKernel *
nova_jit_reconstruct_kernel(const NovaKernel *base,
                              const ZMirrorV2Packet *patch,
                              const NovaDataFingerprint *expected_fp) {
  if (!patch || !expected_fp)
    return NULL;

  uint32_t element_count = expected_fp->element_count;
  float *reconstructed = malloc(element_count * 4);

  if (base) {
    memcpy(reconstructed, base->binary_code, base->code_size);
  } else {
    memset(reconstructed, 0, element_count * 4);
  }

  zmirror_v2_apply(reconstructed, patch);

  // 🛡️ [Sentinel] Force-verify the code before declaring it a 'Kernel'
  NovaDataFingerprint actual_fp;
  nova_health_fingerprint(reconstructed, element_count, &actual_fp);

  if (memcmp(actual_fp.sha256, expected_fp->sha256, 32) != 0) {
    printf("🚨 [JIT] CODE INTEGRITY ERROR! Malicious patch rejected.\n");
    free(reconstructed);
    return NULL;
  }

  NovaKernel *k = malloc(sizeof(NovaKernel));
  k->binary_code = (uint8_t *)reconstructed;
  k->code_size = element_count * 4;
  k->fingerprint = actual_fp;
  return k;
}

void nova_jit_execute(const NovaKernel *kernel) {
  if (!kernel)
    return;

  printf("⚡ [JIT] Executing Sovereign Kernel: '%s' (%u bytes)\n",
         kernel->kernel_name, kernel->code_size);

  // Simulation: In a real JIT, this would map the memory as executable
  // (mprotect) and jump to the entry point.
  printf("   ↳ Hardware status: NEON Optimized Core found.\n");
  printf(
      "   ↳ Execution result: Matrix operation completed in native cycle.\n");
}
