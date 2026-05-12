/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_attest.h — Proof Attestation & Cryptographic Sealing
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_ATTEST_H
#define NOVA_ATTEST_H

#include "../formal/nova_obligation.h"
#include "../formal/nova_policy.h"
#include <stdbool.h>

/**
 * Initialize attestation system
 */
void nova_attest_init(void);

/**
 * Shutdown attestation system
 */
void nova_attest_shutdown(void);

/**
 * Record an attestation (seal the verdict)
 */
int nova_attest_record(const ObligationSet *obligations,
                         const VerdictReport *verdict, const char *policy_mode);

/**
 * Verify the integrity of the attestation chain
 */
bool nova_attest_verify_chain(void);

/**
 * Export attestation chain to JSON
 */
int nova_attest_export_json(const char *path);

/**
 * Print attestation chain summary to stderr
 */
void nova_attest_print_chain(void);

#endif /* NOVA_ATTEST_H */
