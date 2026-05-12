/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_shard.h - Decentralized Model Serving (DMS) Structures
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_SHARD_H
#define NOVA_SHARD_H

#include "nova_cluster.h"
#include "nova_tensor.h"

#define MAX_SHARD_LAYERS 32

typedef struct {
  char shard_id[64];
  uint32_t start_layer;
  uint32_t end_layer;
  NovaTensor *weights[MAX_SHARD_LAYERS];
  uint32_t layer_count;
} NovaModelShard;

typedef struct {
  NovaModelShard *local_shard;
  NovaClusterContext *cluster;
  uint32_t total_model_layers;
} NovaShardExecutor;

// DMS API
NovaShardExecutor *nova_shard_init(NovaClusterContext *cluster,
                                       uint32_t start, uint32_t end);
void nova_shard_load_layer(NovaShardExecutor *exec, uint32_t layer_idx,
                             NovaTensor *w);

/**
 * Executes a partial forward pass on this node.
 * If this is not the last shard, it returns a [Sentinel] signed Mirror packet
 * containing the activations for the next node.
 */
void *nova_shard_forward_step(NovaShardExecutor *exec, NovaTensor *input,
                                uint32_t *out_size);

#endif // NOVA_SHARD_H
