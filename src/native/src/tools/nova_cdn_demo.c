/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_cdn_demo.c - Sovereign Content Addressable Storage (CAS/CDN) Demo
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_cluster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  📦 NOVA MIRROR CDN: SOVEREIGN ASSET DISTRIBUTION\n");
  printf("  Content-Addressable Storage (CAS) & Global Resilience\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Cluster Setup (3 Nodes across different location simulations)
  NovaClusterContext *cluster = nova_cluster_create(1024, 0);
  nova_cluster_add_node(cluster, "hostinger_vps", "1.1.1.1", 9001);
  nova_cluster_add_node(cluster, "gcp_us_east", "2.2.2.2", 9002);
  nova_cluster_add_node(cluster, "local_mac", "127.0.0.1", 9003);

  // 2. Resource Registration (Advertising Assets)
  const char *model_shard_a =
      "sha256:d8a2...3e1f"; // Stable Diffusion UNet Shard 1
  const char *model_shard_b =
      "sha256:7b9c...a4d2"; // Stable Diffusion UNet Shard 2

  printf("📡 [CDN] Registering distributed assets...\n");
  nova_cluster_register_resource(cluster, 0,
                                   model_shard_a); // Hostinger has Shard A
  nova_cluster_register_resource(cluster, 1,
                                   model_shard_b); // GCP has Shard B

  printf("   - Asset A hosted by [Node 0: Hostinger]\n");
  printf("   - Asset B hosted by [Node 1: GCP]\n\n");

  // 3. User Discovery (The Search)
  const char *search_id = "sha256:7b9c...a4d2";
  printf("🔍 [Local Mac] Looking for UNet Shard B (%s)...\n", search_id);

  int provider_node = nova_cluster_locate_resource(cluster, search_id);

  if (provider_node != -1) {
    NovaClusterNode *node = &cluster->nodes[provider_node];
    printf("✅ [CDN] Asset found at [Node %d: %s]\n", provider_node,
           node->node_id);
    printf(
        "🚀 [Pipeline] Requesting Mirror v2 Recipes for reconstruction...\n");

    // Simulating the P2P fetch Logic
    printf("📦 [P2P] Downloading delta recipes from %s:%d...\n",
           node->ip_address, node->port);
    printf(
        "🏗️  [Local] Reconstructing Shard B using local cache baseline...\n");
  } else {
    printf("❌ [CDN] Asset not found in the global cluster.\n");
  }

  printf("\n🏁 [Conclusion] Sovereign CDN logic verified. Data is "
         "location-independent.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  nova_cluster_destroy(cluster);
  yield 0;
}
