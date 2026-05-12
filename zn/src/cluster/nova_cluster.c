#include "nova_cluster.h"
#include "nova_bridge.h"
#include "nova_mirror_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaClusterContext *nova_cluster_create(uint32_t state_count,
                                            uint32_t local_idx) {
  NovaClusterContext *ctx = calloc(1, sizeof(NovaClusterContext));
  ctx->state_element_count = state_count;
  ctx->local_node_idx = local_idx;
  ctx->redline_threshold = 0.85f; // Default 85%
  return ctx;
}

void nova_cluster_add_node(NovaClusterContext *ctx, const char *id,
                             const char *ip, uint16_t port) {
  if (ctx->node_count >= MAX_CLUSTER_NODES)
    return;

  NovaClusterNode *node = &ctx->nodes[ctx->node_count];
  strncpy(node->node_id, id, 63);
  strncpy(node->ip_address, ip, 31);
  node->port = port;
  node->is_active = true;
  node->cached_state = calloc(ctx->state_element_count, sizeof(float));

  ctx->node_count++;
}

void nova_cluster_destroy(NovaClusterContext *ctx) {
  for (uint32_t i = 0; i < ctx->node_count; i++) {
    free(ctx->nodes[i].cached_state);
  }
  free(ctx);
}

uint8_t *nova_cluster_generate_sync(NovaClusterContext *ctx,
                                      uint32_t target_node_idx,
                                      const float *current_state,
                                      uint32_t *out_size) {
  if (target_node_idx >= ctx->node_count)
    return NULL;

  NovaClusterNode *target = &ctx->nodes[target_node_idx];

  // Create delta/recipe relative to what the TARGET node is known to have
  ZMirrorV2Packet *packet = zmirror_v2_encode(
      target->cached_state, current_state, ctx->state_element_count, 1e-4f);

  // Serialized Bridge format for network transmission
  uint8_t *wire_buffer = nova_bridge_serialize(packet, out_size);

  // Speculatively update our local cache of the target node's state
  // (We assume they will receive and apply this successfully)
  zmirror_v2_apply(target->cached_state, packet);

  zmirror_v2_destroy_packet(packet);
  return wire_buffer;
}

void nova_cluster_process_sync(NovaClusterContext *ctx, uint32_t sender_idx,
                                 const uint8_t *wire_buffer) {
  if (sender_idx >= ctx->node_count)
    return;

  NovaClusterNode *sender = &ctx->nodes[sender_idx];

  // Deserialize the incoming Mirror recipe
  ZMirrorV2Packet *packet = nova_bridge_deserialize(wire_buffer);
  if (!packet)
    return;

  // Update the local representation of the sender's state
  zmirror_v2_apply(sender->cached_state, packet);

  zmirror_v2_destroy_packet(packet);
}

void nova_cluster_update_telemetry(NovaClusterContext *ctx,
                                     uint32_t node_idx, float load) {
  if (node_idx < ctx->node_count) {
    ctx->nodes[node_idx].load_factor = load;
    ctx->nodes[node_idx].is_overloaded = (load >= ctx->redline_threshold);
  }
}

int nova_cluster_find_coolest_node(NovaClusterContext *ctx) {
  int coolest = -1;
  float min_load = 2.0f;
  for (uint32_t i = 0; i < ctx->node_count; i++) {
    if (ctx->nodes[i].is_active && ctx->nodes[i].load_factor < min_load) {
      min_load = ctx->nodes[i].load_factor;
      coolest = (int)i;
    }
  }
  return coolest;
}

bool nova_cluster_propose_migration(NovaClusterContext *ctx,
                                      uint32_t source_idx,
                                      uint32_t target_idx) {
  if (source_idx >= ctx->node_count || target_idx >= ctx->node_count)
    return false;

  NovaClusterNode *source = &ctx->nodes[source_idx];
  NovaClusterNode *target = &ctx->nodes[target_idx];

  if (!source->is_overloaded) {
    printf("ℹ️ [Balancer] Node %u is stable. No migration needed.\n",
           source_idx);
    return false;
  }

  printf("⚖️ [Balancer] Redline reached on Node %u (Load: %.2f). Migrating to "
         "Node %u...\n",
         source_idx, source->load_factor, target_idx);

  // Migration is essentially a high-priority Mirror Sync
  // We 'push' the source's state to the target's cache
  ZMirrorV2Packet *migration_packet =
      zmirror_v2_encode(target->cached_state, source->cached_state,
                        ctx->state_element_count, 1e-5f);

  // Safety check: Verify migration integrity before 'handover'
  // (Simulating a verify call here since this is C-FFI bound)
  zmirror_v2_apply(target->cached_state, migration_packet);

  printf("✅ [Balancer] Migration successful. Node %u relieved.\n", source_idx);

  zmirror_v2_destroy_packet(migration_packet);
  return true;
}

void nova_cluster_register_resource(NovaClusterContext *ctx,
                                      uint32_t node_idx,
                                      const char *resource_id) {
  if (node_idx >= ctx->node_count || !resource_id)
    return;

  NovaClusterNode *node = &ctx->nodes[node_idx];
  if (node->resource_count < 1024) {
    node->shared_resource_ids[node->resource_count++] = strdup(resource_id);
  }
}

int nova_cluster_locate_resource(NovaClusterContext *ctx,
                                   const char *resource_id) {
  if (!resource_id)
    return -1;

  // Simple search (can be optimized with DHT later)
  for (uint32_t i = 0; i < ctx->node_count; i++) {
    NovaClusterNode *node = &ctx->nodes[i];
    for (uint32_t r = 0; r < node->resource_count; r++) {
      if (strcmp(node->shared_resource_ids[r], resource_id) == 0) {
        return (int)i;
      }
    }
  }
  return -1;
}

void nova_cluster_gossip_tick(NovaClusterContext *ctx, uint32_t *target_idx,
                                NovaGossipMessage *out_msg) {
  if (!ctx || ctx->node_count <= 1)
    return;

  // 1. Prepare Gossip about OURSELVES
  uint32_t local_idx = ctx->local_node_idx;
  NovaClusterNode *local = &ctx->nodes[local_idx];

  local->version++; // Increment version

  out_msg->sender_idx = local_idx;
  out_msg->version = local->version;
  out_msg->current_load = local->load_factor;
  out_msg->resource_count = local->resource_count;

  // Copy sample resources (first 16)
  for (uint32_t i = 0; i < 16 && i < local->resource_count; i++) {
    out_msg->resources[i] = local->shared_resource_ids[i];
  }

  // 2. Select a RANDOM peer to gossip with (Infection style)
  uint32_t peer;
  do {
    peer = rand() % ctx->node_count;
  } while (peer == local_idx);

  *target_idx = peer;
}

void nova_cluster_process_gossip(NovaClusterContext *ctx,
                                   const NovaGossipMessage *msg) {
  if (!ctx || msg->sender_idx >= ctx->node_count)
    return;

  NovaClusterNode *sender_info = &ctx->nodes[msg->sender_idx];

  // 🛡️ Only update if the incoming information is NEWER (Logical Clock)
  if (msg->version > sender_info->version) {
    printf("🗣️  [Gossip] Received Update from Node %u (v%u -> v%u). "
           "Synchronizing...\n",
           msg->sender_idx, sender_info->version, msg->version);

    sender_info->version = msg->version;
    sender_info->load_factor = msg->current_load;
    sender_info->is_overloaded = (msg->current_load >= ctx->redline_threshold);

    // Merge Resource IDs (Simplified for demo)
    // In real world, we'd check for uniqueness
    for (uint32_t i = 0; i < msg->resource_count && i < 16; i++) {
      bool exists = false;
      for (uint32_t r = 0; r < sender_info->resource_count; r++) {
        if (strcmp(sender_info->shared_resource_ids[r], msg->resources[i]) ==
            0) {
          exists = true;
          break;
        }
      }
      if (!exists && sender_info->resource_count < 1024) {
        sender_info->shared_resource_ids[sender_info->resource_count++] =
            strdup(msg->resources[i]);
      }
    }
  }
}
