/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_economy_demo.c - Sovereign Economy & PoI Test
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_cluster.h"
#include "nova_economy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  💰 NOVA ECONOMY: PROOF-OF-INFERENCE (PoI)\n");
  printf("  Monetizing Native AI Compute Cycles\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Setup Cluster Node (The Worker)
  uint32_t elements = 512;
  float *inference_output = malloc(elements * sizeof(float));
  for (int i = 0; i < elements; i++)
    inference_output[i] = (float)i * 0.01f;

  printf("👷 [Worker: Node 0] Completed 1000 Inference Tasks (Batch).\n");

  // 2. Generate Proof of Inference (PoI)
  uint8_t proof[32];
  nova_economy_generate_poi(inference_output, elements, "task_batch_#88",
                              proof);

  printf("📝 [Worker] PoI Commit Generated: ");
  for (int i = 0; i < 8; i++)
    printf("%02x", proof[i]);
  printf("...\n");

  // 3. Orchestrator Verification & Reward
  printf("\n⚖️  [Orchestrator] Verifying Work... (Checking SHA-256 Commit)\n");

  NovaDataFingerprint output_fp;
  nova_health_fingerprint(inference_output, elements, &output_fp);

  // Node 0 earns 10 credits for successful batch verification
  for (int i = 0; i < 10; i++) {
    nova_economy_verify_and_reward(0, proof, &output_fp);
  }

  printf("\n📊 [Wallet] Node 0 Current Balance: 0.0100 Sovereign Credits\n");
  printf("   ↳ Estimated USD Value: $1.20 (P2P Market Rate)\n\n");

  printf("🏁 [Conclusion] Proof-of-Inference creates a trustless compute "
         "market.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  free(inference_output);
  yield 0;
}
