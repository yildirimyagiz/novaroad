/**
 * tensor_parallel.c - Tensor Parallelism Implementation
 * 
 * Split tensors across multiple GPUs within a single layer
 * Useful for very large layers (e.g., 8192 hidden dim across 8 GPUs)
 */

#include "nova_distributed.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// Tensor Parallelism
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Split tensor along specified dimension
 */
NovaTensorParallel *nova_tensor_parallel_split(
    const NovaTensor *tensor,
    int split_dim,
    int num_gpus
) {
    if (!tensor || split_dim < 0 || split_dim >= tensor->ndim || num_gpus <= 0) {
        return NULL;
    }
    
    NovaTensorParallel *tp = calloc(1, sizeof(NovaTensorParallel));
    if (!tp) return NULL;
    
    tp->num_shards = num_gpus;
    tp->split_dim = split_dim;
    
    // Calculate shard sizes
    int64_t total_size = tensor->shape[split_dim];
    int64_t shard_size = (total_size + num_gpus - 1) / num_gpus;
    
    tp->shard_sizes = malloc(num_gpus * sizeof(int64_t));
    tp->shard_offsets = malloc(num_gpus * sizeof(int64_t));
    tp->shards = malloc(num_gpus * sizeof(NovaTensor*));
    
    // Create shards
    for (int gpu = 0; gpu < num_gpus; gpu++) {
        int64_t offset = gpu * shard_size;
        int64_t size = shard_size;
        
        if (offset + size > total_size) {
            size = total_size - offset;
        }
        
        if (size <= 0) {
            tp->shards[gpu] = NULL;
            tp->shard_sizes[gpu] = 0;
            tp->shard_offsets[gpu] = 0;
            continue;
        }
        
        tp->shard_offsets[gpu] = offset;
        tp->shard_sizes[gpu] = size;
        
        // Create shard with modified shape
        int64_t *shard_shape = malloc(tensor->ndim * sizeof(int64_t));
        memcpy(shard_shape, tensor->shape, tensor->ndim * sizeof(int64_t));
        shard_shape[split_dim] = size;
        
        tp->shards[gpu] = nova_tensor_create(NULL, shard_shape, tensor->ndim, tensor->dtype);
        
        if (tp->shards[gpu]) {
            // Copy data to shard
            const float *src_data = (const float *)tensor->data;
            float *dst_data = (float *)tp->shards[gpu]->data;
            
            // Calculate strides
            int64_t outer_size = 1;
            for (int d = 0; d < split_dim; d++) {
                outer_size *= tensor->shape[d];
            }
            
            int64_t inner_size = 1;
            for (int d = split_dim + 1; d < tensor->ndim; d++) {
                inner_size *= tensor->shape[d];
            }
            
            // Copy data slice by slice
            for (int64_t o = 0; o < outer_size; o++) {
                for (int64_t s = 0; s < size; s++) {
                    for (int64_t i = 0; i < inner_size; i++) {
                        int64_t src_idx = (o * total_size + offset + s) * inner_size + i;
                        int64_t dst_idx = (o * size + s) * inner_size + i;
                        dst_data[dst_idx] = src_data[src_idx];
                    }
                }
            }
            
            printf("✅ Shard %d: offset=%lld, size=%lld\n", gpu, offset, size);
        }
        
        free(shard_shape);
    }
    
    printf("✅ Tensor split across %d GPUs (dim=%d)\n", num_gpus, split_dim);
    
    return tp;
}

/**
 * Gather shards back into single tensor
 */
NovaTensor *nova_tensor_parallel_gather(const NovaTensorParallel *tp) {
    if (!tp || tp->num_shards == 0) return NULL;
    
    // Find a valid shard to get shape info
    NovaTensor *first_shard = NULL;
    for (int i = 0; i < tp->num_shards; i++) {
        if (tp->shards[i]) {
            first_shard = tp->shards[i];
            break;
        }
    }
    
    if (!first_shard) return NULL;
    
    // Calculate full tensor shape
    int64_t *full_shape = malloc(first_shard->ndim * sizeof(int64_t));
    memcpy(full_shape, first_shard->shape, first_shard->ndim * sizeof(int64_t));
    
    // Sum up the split dimension
    int64_t total_split_size = 0;
    for (int i = 0; i < tp->num_shards; i++) {
        total_split_size += tp->shard_sizes[i];
    }
    full_shape[tp->split_dim] = total_split_size;
    
    // Create full tensor
    NovaTensor *full = nova_tensor_create(NULL, full_shape, first_shard->ndim, first_shard->dtype);
    free(full_shape);
    
    if (!full) return NULL;
    
    // Copy shards back
    float *full_data = (float *)full->data;
    
    int64_t outer_size = 1;
    for (int d = 0; d < tp->split_dim; d++) {
        outer_size *= full->shape[d];
    }
    
    int64_t inner_size = 1;
    for (int d = tp->split_dim + 1; d < full->ndim; d++) {
        inner_size *= full->shape[d];
    }
    
    for (int gpu = 0; gpu < tp->num_shards; gpu++) {
        if (!tp->shards[gpu]) continue;
        
        const float *shard_data = (const float *)tp->shards[gpu]->data;
        int64_t offset = tp->shard_offsets[gpu];
        int64_t size = tp->shard_sizes[gpu];
        
        for (int64_t o = 0; o < outer_size; o++) {
            for (int64_t s = 0; s < size; s++) {
                for (int64_t i = 0; i < inner_size; i++) {
                    int64_t dst_idx = (o * total_split_size + offset + s) * inner_size + i;
                    int64_t src_idx = (o * size + s) * inner_size + i;
                    full_data[dst_idx] = shard_data[src_idx];
                }
            }
        }
    }
    
    printf("✅ Gathered tensor from %d shards\n", tp->num_shards);
    
    return full;
}

/**
 * All-reduce across tensor parallel group
 */
int nova_tensor_parallel_all_reduce(NovaTensor *tensor) {
    if (!tensor) return -1;
    
    // TODO: Implement actual GPU communication
    // For now, this is a no-op
    
    printf("⚠️  All-reduce not yet implemented (requires MPI/NCCL)\n");
    return 0;
}

/**
 * Free tensor parallel resources
 */
void nova_tensor_parallel_destroy(NovaTensorParallel *tp) {
    if (!tp) return;
    
    if (tp->shards) {
        for (int i = 0; i < tp->num_shards; i++) {
            if (tp->shards[i]) {
                nova_tensor_destroy(tp->shards[i]);
            }
        }
        free(tp->shards);
    }
    
    free(tp->shard_offsets);
    free(tp->shard_sizes);
    free(tp);
}

// ═══════════════════════════════════════════════════════════════════════════
// Distributed Configuration
// ═══════════════════════════════════════════════════════════════════════════

static NovaDistributedConfig g_config = {0};

int nova_distributed_init(NovaDistributedConfig *config) {
    if (!config) return -1;
    
    // Auto-detect GPUs
    // TODO: Query actual GPU count from CUDA/Metal
    int num_gpus = 1;  // Default
    
    #ifdef __CUDACC__
    // CUDA GPU detection would go here
    #endif
    
    config->world_size = num_gpus;
    config->rank = 0;
    config->local_rank = 0;
    
    g_config = *config;
    
    printf("✅ Distributed initialized: %d GPUs, rank=%d\n",
           config->world_size, config->rank);
    
    return 0;
}

void nova_distributed_cleanup(void) {
    memset(&g_config, 0, sizeof(g_config));
}

const NovaDistributedConfig *nova_distributed_get_config(void) {
    return &g_config;
}

/**
 * Suggest optimal configuration
 */
NovaDistributedConfig nova_dist_suggest_config(
    int num_layers,
    int64_t model_size_bytes,
    int num_gpus
) {
    NovaDistributedConfig config = {0};
    
    config.world_size = num_gpus;
    
    // Simple heuristic
    if (num_gpus == 1) {
        config.type = NOVA_PARALLEL_NONE;
    } else if (num_gpus <= 4) {
        // Tensor parallelism works well for 2-4 GPUs
        config.type = NOVA_PARALLEL_TENSOR;
        config.tensor_parallel_size = num_gpus;
    } else {
        // Hybrid: tensor + pipeline for 8+ GPUs
        config.type = NOVA_PARALLEL_HYBRID;
        config.tensor_parallel_size = 4;
        config.pipeline_parallel_size = num_gpus / 4;
    }
    
    printf("💡 Suggested config: %d GPUs, type=%d\n", num_gpus, config.type);
    
    return config;
}

void nova_dist_print_config(const NovaDistributedConfig *config) {
    if (!config) return;
    
    printf("═══ Distributed Configuration ═══\n");
    printf("  World size: %d\n", config->world_size);
    printf("  Rank: %d\n", config->rank);
    printf("  Type: %d\n", config->type);
    printf("  Tensor parallel: %d\n", config->tensor_parallel_size);
    printf("  Pipeline parallel: %d\n", config->pipeline_parallel_size);
}
