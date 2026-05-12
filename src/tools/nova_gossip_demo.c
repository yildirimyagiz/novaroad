/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_gossip_demo.c - Sovereign Federated Discovery Demo
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_cluster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() {
  srand(time(NULL));
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  🗣️  NOVA GOSSIP PROTOCOL: FEDERATED DISCOVERY\n");
  printf("  Decentralized Heartbeats & Resource Propagation\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Cluster Setup (4 Nodes)
  NovaClusterContext *cluster =
      nova_cluster_create(1024, 0); // local_node = Node 0
  nova_cluster_add_node(cluster, "node_alpha", "127.0.0.1", 9001); // 0
  nova_cluster_add_node(cluster, "node_beta", "127.0.0.1", 9002);  // 1
  nova_cluster_add_node(cluster, "node_gamma", "127.0.0.1", 9003); // 2
  nova_cluster_add_node(cluster, "node_delta", "127.0.0.1", 9004); // 3

  // 2. Local Setup (Node 0 Has some private knowledge)
  nova_cluster_register_resource(cluster, 0, "sha256:alpha_weights_v2");
  cluster->nodes[0].load_factor = 0.42f;

  printf(
      "🚀 [Node 0] Local State: Load=42%%, Assets=1. Starting Gossip...\n\n");

  // 3. Simulating Multiple Gossip Rounds (Infection style)
  for (int round = 1; round <= 3; round++) {
    printf("🔄 [ROUND %d] ----------------------------------\n", round);

    uint32_t target_idx;
    NovaGossipMessage msg;

    // Node 0 Gossips to someone random
    nova_cluster_gossip_tick(cluster, &target_idx, &msg);

    printf("📤 [Node 0] Spreading gossip to Node %u...\n", target_idx);

    // Process at target
    nova_cluster_process_gossip(cluster, &msg);

    // Simulation: Now the target also spreading the news to someone ELSE
    uint32_t secondary_target;
    NovaGossipMessage secondary_msg;

    // Temporarily change local_node_idx to simulate target's point of view
    uint32_t original_local = cluster->local_node_idx;
    cluster->local_node_idx = target_idx;

    nova_cluster_gossip_tick(cluster, &secondary_target, &secondary_msg);
    printf("📢 [Node %u] Passing the gossip to Node %u...\n", target_idx,
           secondary_target);

    // Back to original local for processing
    cluster->local_node_idx = original_local;
    nova_cluster_process_gossip(cluster, &secondary_msg);

    printf("\n");
  }

  // 4. Final Verification
  printf("📊 [Final Health Check] Cluster Knowledge Propagation:\n");
  for (int i = 0; i < 4; i++) {
    NovaClusterNode *n = &cluster->nodes[i];
    printf("   - Node %d (%s): v%u, Load: %.1f%%, Known Assets: %u\n", i,
           n->node_id, n->version, n->load_factor * 100.0, n->resource_count);
  }

  printf(
      "\n✅ [Gossip] Knowledge successfully federated across the cluster.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  nova_cluster_destroy(cluster);
  return 0;
}
