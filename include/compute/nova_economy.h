/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_economy.h - Sovereign Economy & Proof-of-Inference
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_ECONOMY_H
#define NOVA_ECONOMY_H

#include "nova_health.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  NOVA_LICENSE_FREE,
  NOVA_LICENSE_FOUNDING_MASTER,
  NOVA_LICENSE_ENTERPRISE
} NovaLicenseType;

typedef struct {
  uint64_t total_inference_count;
  double sovereign_credits;
  uint8_t wallet_address[32];
  uint32_t reliability_score; // 0-1000
  NovaLicenseType license;
  bool is_master_node;
} NovaEconomicProfile;

/**
 * Generates a proof for a completed inference task.
 * The proof ties the input, output, and node identity into a SHA-256 commit.
 */
void nova_economy_generate_poi(const float *output, uint32_t count,
                                 const char *task_id, uint8_t *out_proof);

/**
 * Verifies a proof from another node and awards credits if valid.
 */
bool nova_economy_verify_and_reward(uint32_t node_idx, const uint8_t *proof,
                                      const NovaDataFingerprint *output_fp);

// 🛒 Phase 6: Marketplace & Revenue
bool nova_economy_purchase_license(NovaEconomicProfile *profile,
                                     NovaLicenseType type);
void nova_economy_distribute_revenue(NovaEconomicProfile *profiles,
                                       uint32_t count, double total_usd);

#endif // NOVA_ECONOMY_H
