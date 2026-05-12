#include "nova_cluster.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bench_cluster_p2p() {
  uint32_t state_elements = 10000;
  NovaClusterContext *cluster = nova_cluster_create(state_elements, 0);

  // Add 3 nodes to our sovereign cluster
  nova_cluster_add_node(cluster, "node-alpha", "10.0.0.1", 8080);
  nova_cluster_add_node(cluster, "node-beta", "10.0.0.2", 8080);
  nova_cluster_add_node(cluster, "node-gamma", "10.0.0.3", 8080);

  float *local_state = calloc(state_elements, sizeof(float));

  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  Nova Cluster Sovereignty: P2P Mirror Sync\n");
  printf("  Cluster Size: %u Nodes\n", cluster->node_count);
  printf("  State per Node: %.2f KB\n", (state_elements * 4.0) / 1024.0);
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. Initial State: All zeros.
  // 2. Local State Change: Node Alpha (us) updates state.
  for (int i = 0; i < 100; i++)
    local_state[i] = 1.23f;

  // 3. Sync Alpha -> Beta
  uint32_t wire_size = 0;
  uint8_t *buffer =
      nova_cluster_generate_sync(cluster, 1, local_state, &wire_size);
  printf("  [ALPHA -> BETA] Sync Generated: %u bytes\n", wire_size);

  // 4. Beta processes the sync
  nova_cluster_process_sync(cluster, 1, buffer);
  printf("  [BETA] State updated from Mirror Recipe.\n");

  // 5. Check if Beta's cached state in Alpha matches local state
  int errors = 0;
  for (uint32_t i = 0; i < state_elements; i++) {
    if (fabsf(cluster->nodes[1].cached_state[i] - local_state[i]) > 1e-5f)
      errors++;
  }

  printf("  [P2P VERIFY] %s\n",
         errors == 0 ? "NODES IN SYNC ✅" : "NODES DRIFTED ❌");

  // 6. Efficiency Check
  uint32_t raw_size = state_elements * sizeof(float);
  printf("  [EFFICIENCY] Bandwidth Saving: %.1f x\n",
         (double)raw_size / wire_size);
  printf("════════════════════════════════════════════════════════════\n\n");

  free(buffer);
  free(local_state);
  nova_cluster_destroy(cluster);
}

int main() {
  bench_cluster_p2p();
  yield 0;
}
