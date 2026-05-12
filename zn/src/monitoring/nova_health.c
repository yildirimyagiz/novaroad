/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_health.c - Data Integrity Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_health.h"
#include "nova_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void nova_health_fingerprint(const float *data, uint32_t count,
                               NovaDataFingerprint *out_fp) {
  if (!data || !out_fp)
    return;

  NovaSHA256Ctx ctx;
  nova_sha256_init(&ctx);

  uint32_t xor_sum = 0;
  const uint32_t *raw_bits = (const uint32_t *)data;

  // Fast multi-pass fingerprinting
  for (uint32_t i = 0; i < count; i++) {
    xor_sum ^= raw_bits[i];
  }

  nova_sha256_update(&ctx, (const uint8_t *)data, count * sizeof(float));
  nova_sha256_final(&ctx, out_fp->sha256);

  out_fp->element_count = count;
  out_fp->checksum_xor = xor_sum;
}

bool nova_health_verify_sync_integrity(
    const float *prev, const ZMirrorV2Packet *packet,
    const NovaDataFingerprint *expected_fp) {
  if (!prev || !packet || !expected_fp)
    return false;

  // 1. Memory Allocation for 'Shadow Apply'
  // We never apply to the main state unless the shadow is perfect.
  uint32_t size = expected_fp->element_count * sizeof(float);
  float *shadow_copy = malloc(size);
  memcpy(shadow_copy, prev, size);

  // 2. Apply the recipes to the shadow memory
  zmirror_v2_apply(shadow_copy, packet);

  // 3. Verify the final state
  NovaDataFingerprint shadow_fp;
  nova_health_fingerprint(shadow_copy, expected_fp->element_count,
                            &shadow_fp);

  bool is_valid = true;
  if (shadow_fp.checksum_xor != expected_fp->checksum_xor) {
    printf("⚠️ [Health] Checksum XOR Mismatch! Expected %08x, got %08x\n",
           expected_fp->checksum_xor, shadow_fp.checksum_xor);
    is_valid = false;
  } else if (memcmp(shadow_fp.sha256, expected_fp->sha256, 32) != 0) {
    printf("🚨 [Health] SHA-256 Integrity Violation! Data corruption detected "
           "during sync.\n");
    is_valid = false;
  }

  free(shadow_copy);
  return is_valid;
}

bool nova_health_audit_file(const char *path, const uint8_t *expected_hash) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  NovaSHA256Ctx ctx;
  nova_sha256_init(&ctx);

  uint8_t buffer[4096];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
    nova_sha256_update(&ctx, buffer, bytes);
  }
  fclose(f);

  uint8_t actual_hash[32];
  nova_sha256_final(&ctx, actual_hash);

  return memcmp(actual_hash, expected_hash, 32) == 0;
}
