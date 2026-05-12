/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_cluster.h - P2P Cluster Sovereignty & Decentralized Sync
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_CLUSTER_H
#define NOVA_CLUSTER_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_CLUSTER_NODES 256

typedef struct {
  char node_id[64];
  char ip_address[32];
  uint16_t port;
  bool is_active;
  uint32_t last_sync_ts;
  float *cached_state; // The last known state of this node

  // 📈 Telemetry & Load Balancing
  float load_factor;  // 0.0 to 1.0 (Stress level)
  uint32_t req_count; // Request throughput
  bool is_overloaded; // Above Redline

  // 📦 Distributed Assets (CDN)
  char *shared_resource_ids[1024]; // List of SHA-256 IDs this node has
  uint32_t resource_count;

  // 🗣️ Gossip Protocol
  uint64_t last_heartbeat;
  uint32_t version; // Logical clock for metadata freshness
} NovaClusterNode;

typedef struct {
  NovaClusterNode nodes[MAX_CLUSTER_NODES];
  uint32_t node_count;
  uint32_t local_node_idx;
  uint32_t state_element_count;

  // ⚖️ Load Balancing Config
  float redline_threshold; // e.g., 0.85
} NovaClusterContext;

// Cluster Lifecycle
NovaClusterContext *nova_cluster_create(uint32_t state_count,
                                            uint32_t local_idx);
void nova_cluster_add_node(NovaClusterContext *ctx, const char *id,
                             const char *ip, uint16_t port);
void nova_cluster_destroy(NovaClusterContext *ctx);

// P2P Sovereignty: Syncing states between nodes using Mirror v2 Recipes
// This generates a packet for a specific target node based on what we think
// they know
uint8_t *nova_cluster_generate_sync(NovaClusterContext *ctx,
                                      uint32_t target_node_idx,
                                      const float *current_state,
                                      uint32_t *out_size);

// Processing an incoming sync packet from another node
void nova_cluster_process_sync(NovaClusterContext *ctx, uint32_t sender_idx,
                                 const uint8_t *wire_buffer);

// ⚖️ Phase 4.2: Dynamic Load Balancing
void nova_cluster_update_telemetry(NovaClusterContext *ctx,
                                     uint32_t node_idx, float load);
int nova_cluster_find_coolest_node(
    NovaClusterContext *ctx); // Find node with lowest load
bool nova_cluster_propose_migration(NovaClusterContext *ctx,
                                      uint32_t source_idx, uint32_t target_idx);

// 📦 Phase 4.3: Global Mirror CDN
void nova_cluster_register_resource(NovaClusterContext *ctx,
                                      uint32_t node_idx,
                                      const char *resource_id);
int nova_cluster_locate_resource(NovaClusterContext *ctx,
                                   const char *resource_id);

// 🗣️ Phase 4.4: Gossip & Federated Discovery
typedef struct {
  uint32_t sender_idx;
  uint32_t version;
  float current_load;
  uint32_t resource_count;
  char *resources[16]; // Samples of hosted assets
} NovaGossipMessage;

void nova_cluster_gossip_tick(NovaClusterContext *ctx, uint32_t *target_idx,
                                NovaGossipMessage *out_msg);
void nova_cluster_process_gossip(NovaClusterContext *ctx,
                                   const NovaGossipMessage *msg);

#endif // NOVA_CLUSTER_H
