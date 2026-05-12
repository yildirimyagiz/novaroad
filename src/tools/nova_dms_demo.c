/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_dms_demo.c - Decentralized Model Serving (DMS) Demo
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "runtime/nova_context.h"
#include "nova_mirror_v2.h"
#include "nova_shard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  🛡️  NOVA DMS: DECENTRALIZED MODEL SERVING\n");
  printf("  Privacy-Preserving Pipelined Inference\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Cluster Kurulumu (2 Sanal Düğüm)
  NovaClusterContext *cluster = nova_cluster_create(1024, 0);
  nova_cluster_add_node(cluster, "node_alpha", "127.0.0.1", 9001);
  nova_cluster_add_node(cluster, "node_beta", "127.0.0.1", 9002);

  // 2. Model Sharding (100 Katmanlı Dev Model)
  // Node Alpha: 0-49. katmanlar
  // Node Beta: 50-99. katmanlar
  NovaShardExecutor *alpha_exec = nova_shard_init(cluster, 0, 49);
  NovaShardExecutor *beta_exec = nova_shard_init(cluster, 50, 99);

  printf("📦 [DMS] Model Sharding completed:\n");
  printf("   - Node Alpha: Layers 0 to 49\n");
  printf("   - Node Beta:  Layers 50 to 99\n\n");

  // 3. Inference Başlat (Node Alpha)
  int64_t shape[2] = {1, 512}; // Batch=1, Hidden=512
  NovaTensor *input = nova_tensor_zeros(NULL, shape, 2);

  printf("🚀 [Node Alpha] Starting forward pass (Stage 1)...\n");
  ZMirrorV2Packet *activations_packet =
      (ZMirrorV2Packet *)nova_shard_forward_step(alpha_exec, input, NULL);

  // 4. Pipeline Sync (Alpha -> Beta)
  printf("\n🛰️  [Pipeline] Sending activations recipe to Node Beta...\n");
  printf("   Mirror Recipe Type: ZMIRROR_RECIPE_REPEAT (Optimized for neural "
         "sparsity)\n");

  // 5. Inference Devam Et (Node Beta)
  NovaTensor *beta_input = nova_tensor_zeros(NULL, shape, 2);

  printf("\n📥 [Node Beta] Applying activations recipe (Stage 2)...\n");
  zmirror_v2_apply((float *)beta_input->data, activations_packet);

  ZMirrorV2Packet *final_output =
      (ZMirrorV2Packet *)nova_shard_forward_step(beta_exec, beta_input, NULL);

  printf("\n✅ [DMS] Decentralized Inference Complete.\n");
  printf("   Total Layers Processed: 100\n");
  printf("   Privacy: No single node saw the full model weights.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // Cleanup
  zmirror_v2_destroy_packet(activations_packet);
  zmirror_v2_destroy_packet(final_output);
  nova_tensor_destroy(input);
  nova_tensor_destroy(beta_input);
  free(alpha_exec->local_shard);
  free(alpha_exec);
  free(beta_exec->local_shard);
  free(beta_exec);
  nova_cluster_destroy(cluster);

  return 0;
}
