/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_health.h - Reliability & Data Integrity Guard
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_HEALTH_H
#define NOVA_HEALTH_H

#include "../ml/nova_mirror_v2.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t sha256[32];
  uint32_t element_count;
  uint32_t checksum_xor; // Fast XOR checksum for quick checks
} NovaDataFingerprint;

/**
 * Generates a unique fingerprint for a block of data (tensor/state).
 */
void nova_health_fingerprint(const float *data, uint32_t count,
                               NovaDataFingerprint *out_fp);

/**
 * Verifies if a [Sentinel] packet truly reconstructs the target state.
 * Performs a 'Shadow Apply' in temporary memory to ensure 100% accuracy.
 */
bool nova_health_verify_sync_integrity(
    const float *prev, const ZMirrorV2Packet *packet,
    const NovaDataFingerprint *expected_fp);

/**
 * Audits a file on disk to ensure it hasn't been tampered with or corrupted.
 */
bool nova_health_audit_file(const char *path, const uint8_t *expected_hash);

#endif // NOVA_HEALTH_H
