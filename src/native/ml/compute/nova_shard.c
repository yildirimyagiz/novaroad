/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_shard.c - Decentralized Model Serving Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_shard.h"
#include "nova_mirror_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaShardExecutor *nova_shard_init(NovaClusterContext *cluster,
                                       uint32_t start, uint32_t end) {
  NovaShardExecutor *exec = calloc(1, sizeof(NovaShardExecutor));
  exec->cluster = cluster;
  exec->local_shard = calloc(1, sizeof(NovaModelShard));
  exec->local_shard->start_layer = start;
  exec->local_shard->end_layer = end;
  snprintf(exec->local_shard->shard_id, 64, "shard_%u_%u", start, end);
  return exec;
}

void nova_shard_load_layer(NovaShardExecutor *exec, uint32_t layer_idx,
                             NovaTensor *w) {
  if (!exec || !exec->local_shard)
    return;
  uint32_t internal_idx = layer_idx - exec->local_shard->start_layer;
  if (internal_idx < MAX_SHARD_LAYERS) {
    exec->local_shard->weights[internal_idx] = w;
    if (internal_idx >= exec->local_shard->layer_count) {
      exec->local_shard->layer_count = internal_idx + 1;
    }
  }
}

void *nova_shard_forward_step(NovaShardExecutor *exec, NovaTensor *input,
                                uint32_t *out_size) {
  if (!exec || !input)
    return NULL;

  printf("🧠 [Nova Shard] Executing layers %u to %u...\n",
         exec->local_shard->start_layer, exec->local_shard->end_layer);

  // 1. Local Forward Pass (Simulated)
  // In a real scenario, this would call nova_op_matmul etc.
  // For the demo, we transform the input slightly to simulate processing
  float *data = (float *)input->data;
  for (size_t i = 0; i < input->total_elements; i++) {
    data[i] = data[i] * 0.9f + 0.1f; // Simulated nonlinear activation
  }

  // 2. Activation Compression (Privacy-Preserving Sync)
  // We treat the activations as a 'state' that needs to be mirrored to the next
  // node. Since activations in neural networks often have strong patterns
  // (sparsity, etc.), Mirror v2 is perfect here.

  float *zero_base = calloc(input->total_elements, sizeof(float));
  ZMirrorV2Packet *packet =
      zmirror_v2_encode(zero_base, data, input->total_elements, 1e-4f);
  free(zero_base);

  // 3. Metadata for the next node
  packet->timestamp = 123456789; // To be replaced with real time
  packet->sequence =
      exec->local_shard
          ->end_layer; // Sequence indicates where we are in the model

  printf("📤 [Nova Shard] Pipeline Activations: Compressed to %.2f KB\n",
         (packet->num_recipes * 12.0 + packet->patch_size) / 1024.0);

  return packet; // Next node will apply this to its input buffer
}
