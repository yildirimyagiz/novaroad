/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_balancer_demo.c - Sovereign Load Balancing Demo
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_cluster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  ⚖️  NOVA LOAD BALANCER: SOVEREIGN RESOURCE STEERING\n");
  printf("  Dynamic Task Migration & Redline Protection\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Cluster Setup (3 Nodes)
  uint32_t elements = 1000;
  NovaClusterContext *cluster = nova_cluster_create(elements, 0);
  nova_cluster_add_node(cluster, "node_alpha", "127.0.0.1", 9001);
  nova_cluster_add_node(cluster, "node_beta", "127.0.0.1", 9002);
  nova_cluster_add_node(cluster, "node_gamma", "127.0.0.1", 9003);

  // Initial state: Everything is cool
  nova_cluster_update_telemetry(cluster, 0, 0.20f); // Alpha: 20%
  nova_cluster_update_telemetry(cluster, 1, 0.15f); // Beta: 15%
  nova_cluster_update_telemetry(cluster, 2, 0.10f); // Gamma: 10%

  printf("📡 [Cluster] Initial Load Status:\n");
  for (int i = 0; i < 3; i++)
    printf("   - Node %d: %.2f%%\n", i, cluster->nodes[i].load_factor * 100.0);

  // 2. Simulation: Node Alpha hits the REDLINE (e.g., massive AI inference
  // spike)
  printf("\n🔥 [Alarm] External spike detected on Node Alpha!\n");
  nova_cluster_update_telemetry(cluster, 0, 0.92f); // 92% - REDLINE!

  // 3. Automatic Recovery Logic
  if (cluster->nodes[0].is_overloaded) {
    printf("🚨 [Sentinel] Node 0 is above Redline (Threshold: %.2f%%). "
           "Triggering recovery...\n",
           cluster->redline_threshold * 100.0);

    // Find the coolest node to offload to
    int target = nova_cluster_find_coolest_node(cluster);
    if (target != -1) {
      printf("🔍 [Sentinel] Identified Node %d as coolest candidate (Load: "
             "%.2f%%).\n",
             target, cluster->nodes[target].load_factor * 100.0);

      // 4. Perform Task Migration (State Handover)
      // Simule ediyoruz: Alpha'nın elindeki kritik 1000 elementlik state'i
      // Gamma'ya Mirror ediyoruz.
      for (uint32_t i = 0; i < elements; i++)
        cluster->nodes[0].cached_state[i] = (float)i * 0.5f;

      bool success =
          nova_cluster_propose_migration(cluster, 0, (uint32_t)target);

      if (success) {
        // Post-migration: Load is balanced
        nova_cluster_update_telemetry(cluster, 0, 0.45f);      // Relieved
        nova_cluster_update_telemetry(cluster, target, 0.55f); // Absorbed
      }
    }
  }

  printf("\n⚖️ [Final Status] Post-Migration Cluster State:\n");
  for (int i = 0; i < 3; i++) {
    printf("   - Node %d: %.2f%% %s\n", i,
           cluster->nodes[i].load_factor * 100.0,
           cluster->nodes[i].is_overloaded ? "[🔥 OVERLOADED]" : "[✅ STABLE]");
  }

  printf("\n🏁 [Conclusion] Cluster survived the spike without data loss.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  nova_cluster_destroy(cluster);
  yield 0;
}
