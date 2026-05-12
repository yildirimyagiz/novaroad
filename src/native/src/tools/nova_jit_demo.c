/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_jit_demo.c - JIT-over-Mirror Live Code Distribution
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_jit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  ⚡ NOVA JIT: OVER-THE-AIR NATIVE KERNELS\n");
  printf("  Sovereign Code Mobility & [Sentinel] Integrity\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Define a 'Native Kernel' (Simulated binary code)
  // In reality, this would be compiled machine code for ARM/x86.
  uint32_t code_size = 1024;
  uint8_t *v1_binary = malloc(code_size);
  uint8_t *v2_binary = malloc(code_size);

  // Fill v1 with some base instructions
  for (int i = 0; i < code_size; i++)
    v1_binary[i] = 0x90; // NOP

  // v2 has an 'Optimization Patch' (e.g., NEON MatMul logic)
  memcpy(v2_binary, v1_binary, code_size);
  v2_binary[100] = 0xEB; // JMP / Native optimization instruction
  v2_binary[101] = 0xFE;

  NovaKernel k_v1 = {.code_size = code_size, .binary_code = v1_binary};
  strcpy(k_v1.kernel_name, "MatMul_Base");

  NovaKernel k_v2 = {.code_size = code_size, .binary_code = v2_binary};
  strcpy(k_v2.kernel_name, "MatMul_NEON_v2");

  // 2. Generate a [Mirror Code Delta]
  printf("🛰️  [Cluster] Node Alpha is sending 'Optimization Patch' to Node "
         "Beta...\n");
  ZMirrorV2Packet *code_patch = nova_jit_encode_kernel(&k_v1, &k_v2);

  // Register expected fingerprint for the new code
  NovaDataFingerprint v2_fp;
  nova_health_fingerprint((float *)v2_binary, code_size / 4, &v2_fp);

  printf("   ↳ Code Delta Size: %u bytes (Total binary: %u bytes)\n",
         code_patch->num_recipes * (uint32_t)sizeof(ZMirrorRecipe) +
             code_patch->patch_size,
         code_size);
  printf("   ↳ Effective bandwidth saving: %.1f x\n",
         (float)code_size / (code_patch->patch_size + 1));

  // 3. Node Beta receives and reconstructs
  printf("\n📥 [Node Beta] Receiving patch. Verifying code [Sentinel]...\n");

  NovaKernel *reconstructed =
      nova_jit_reconstruct_kernel(&k_v1, code_patch, &v2_fp);

  if (reconstructed) {
    printf("✅ [Node Beta] Code Integrity Verified. JIT Entry Point Active.\n");
    strcpy(reconstructed->kernel_name, "MatMul_NEON_v2_Shadowed");

    // 4. Execution
    nova_jit_execute(reconstructed);
  } else {
    printf("❌ [Node Beta] MISSION ABORTED: The incoming code patch was "
           "corrupted or malicious.\n");
  }

  printf("\n🏁 [Conclusion] JIT-over-Mirror allows cluster-level code "
         "evolution.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  zmirror_v2_destroy_packet(code_patch);
  free(v1_binary);
  free(v2_binary);
  // free(reconstructed->binary_code); // Handled by free
  yield 0;
}
