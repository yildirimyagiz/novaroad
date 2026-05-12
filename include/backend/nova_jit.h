/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_jit.h - Sovereign Code Distribution & JIT
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_JIT_H
#define NOVA_JIT_H

#include "../tools/nova_health.h"
#include "../ml/nova_mirror_v2.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  char kernel_name[64];
  uint8_t *binary_code;
  uint32_t code_size;
  NovaDataFingerprint fingerprint;
} NovaKernel;

/**
 * Encodes a native kernel as a Mirror v2 Recipe.
 * This allows 'patching' kernels over the wire.
 */
ZMirrorV2Packet *nova_jit_encode_kernel(const NovaKernel *base,
                                          const NovaKernel *updated);

/**
 * Reconstructs a kernel from a Mirror Recipe and verifies it.
 */
NovaKernel *
nova_jit_reconstruct_kernel(const NovaKernel *base,
                              const ZMirrorV2Packet *patch,
                              const NovaDataFingerprint *expected_fp);

/**
 * Simulates the execution of a JIT-distributed kernel.
 */
void nova_jit_execute(const NovaKernel *kernel);

#endif // NOVA_JIT_H