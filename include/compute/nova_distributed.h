/**
 * nova_distributed.h - Distributed Inference
 * 
 * Multi-GPU support with tensor parallelism and pipeline parallelism
 * Enables inference on models larger than single GPU memory
 */

#ifndef NOVA_DISTRIBUTED_H
#define NOVA_DISTRIBUTED_H

#include "../ml/nova_tensor.h"
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// Multi-GPU Configuration
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_PARALLEL_NONE = 0,
    NOVA_PARALLEL_TENSOR,      // Tensor parallelism (split within layer)
    NOVA_PARALLEL_PIPELINE,    // Pipeline parallelism (split across layers)
    NOVA_PARALLEL_DATA,        // Data parallelism (split batch)
    NOVA_PARALLEL_HYBRID       // Combination of above
} NovaParallelismType;

typedef struct {
    int world_size;            // Total number of GPUs
    int rank;                  // Current GPU rank (0 to world_size-1)
    int local_rank;            // GPU rank on this node
    NovaParallelismType type;
    int tensor_parallel_size;  // Number of GPUs for tensor parallelism
    int pipeline_parallel_size; // Number of GPUs for pipeline parallelism
} NovaDistributedConfig;

/**
 * Initialize distributed environment
 * Auto-detects available GPUs
 */
int nova_distributed_init(NovaDistributedConfig *config);

/**
 * Cleanup distributed resources
 */
void nova_distributed_cleanup(void);

/**
 * Get current distributed config
 */
const NovaDistributedConfig *nova_distributed_get_config(void);

// ═══════════════════════════════════════════════════════════════════════════
// Tensor Parallelism
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    NovaTensor **shards;       // Tensor shards on each GPU
    int num_shards;
    int64_t *shard_offsets;    // Offset of each shard in original tensor
    int64_t *shard_sizes;      // Size of each shard
    int split_dim;             // Dimension along which to split
} NovaTensorParallel;

/**
 * Split tensor across GPUs (tensor parallelism)
 * 
 * For attention: split heads across GPUs
 * For FFN: split intermediate dimension
 */
NovaTensorParallel *nova_tensor_parallel_split(
    const NovaTensor *tensor,
    int split_dim,
    int num_gpus
);

/**
 * Gather shards from all GPUs
 */
NovaTensor *nova_tensor_parallel_gather(const NovaTensorParallel *tp);

/**
 * All-reduce across tensor parallel group
 * Synchronizes gradients or activations
 */
int nova_tensor_parallel_all_reduce(NovaTensor *tensor);

/**
 * Free tensor parallel resources
 */
void nova_tensor_parallel_destroy(NovaTensorParallel *tp);

// ═══════════════════════════════════════════════════════════════════════════
// Pipeline Parallelism
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int num_stages;            // Number of pipeline stages
    int stage_id;              // Current stage ID
    int *layer_assignment;     // Which layers belong to which stage
    int num_layers;
    int micro_batch_size;      // Size of micro-batches
} NovaPipelineParallel;

/**
 * Create pipeline parallelism schedule
 * 
 * Divides model layers across GPUs
 * GPU 0: layers 0-7
 * GPU 1: layers 8-15
 * etc.
 */
NovaPipelineParallel *nova_pipeline_parallel_create(
    int num_layers,
    int num_stages,
    int micro_batch_size
);

/**
 * Send activation to next pipeline stage
 */
int nova_pipeline_send_forward(
    const NovaTensor *activation,
    int dst_stage
);

/**
 * Receive activation from previous pipeline stage
 */
NovaTensor *nova_pipeline_recv_forward(int src_stage);

/**
 * Send gradient to previous pipeline stage (backward)
 */
int nova_pipeline_send_backward(
    const NovaTensor *gradient,
    int dst_stage
);

/**
 * Receive gradient from next pipeline stage (backward)
 */
NovaTensor *nova_pipeline_recv_backward(int src_stage);

/**
 * Free pipeline parallel resources
 */
void nova_pipeline_parallel_destroy(NovaPipelineParallel *pp);

// ═══════════════════════════════════════════════════════════════════════════
// Communication Primitives
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Broadcast tensor from root GPU to all GPUs
 */
int nova_dist_broadcast(NovaTensor *tensor, int root_rank);

/**
 * All-reduce (sum/avg) across all GPUs
 */
int nova_dist_all_reduce(NovaTensor *tensor, const char *op);

/**
 * All-gather: gather tensors from all GPUs
 */
NovaTensor **nova_dist_all_gather(const NovaTensor *tensor);

/**
 * Scatter: split and distribute tensor to all GPUs
 */
NovaTensor *nova_dist_scatter(const NovaTensor *tensor, int root_rank);

/**
 * Point-to-point send
 */
int nova_dist_send(const NovaTensor *tensor, int dst_rank);

/**
 * Point-to-point receive
 */
NovaTensor *nova_dist_recv(int src_rank);

/**
 * Barrier synchronization
 */
void nova_dist_barrier(void);

// ═══════════════════════════════════════════════════════════════════════════
// Memory Management
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    size_t total_gpu_memory;   // Total GPU memory available
    size_t used_memory;        // Currently used
    size_t reserved_memory;    // Reserved for caching
    int num_gpus;
} NovaDistributedMemoryStats;

/**
 * Get memory statistics across all GPUs
 */
NovaDistributedMemoryStats nova_dist_get_memory_stats(void);

/**
 * Balance memory across GPUs
 * Moves tensors to balance memory usage
 */
int nova_dist_balance_memory(void);

// ═══════════════════════════════════════════════════════════════════════════
// Model Sharding
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    void *model;               // Model pointer (implementation-specific)
    int num_layers;
    int layers_per_gpu;
    int *gpu_assignment;       // Which GPU owns which layer
} NovaShardedModel;

/**
 * Shard model across multiple GPUs
 * 
 * Automatically determines optimal sharding strategy
 * based on model size and available GPUs
 */
NovaShardedModel *nova_shard_model(
    void *model,
    int num_layers,
    const NovaDistributedConfig *config
);

/**
 * Run inference on sharded model
 */
NovaTensor *nova_sharded_model_forward(
    NovaShardedModel *sharded_model,
    const NovaTensor *input
);

/**
 * Free sharded model
 */
void nova_sharded_model_destroy(NovaShardedModel *model);

// ═══════════════════════════════════════════════════════════════════════════
// Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Get optimal parallelism configuration for model
 */
NovaDistributedConfig nova_dist_suggest_config(
    int num_layers,
    int64_t model_size_bytes,
    int num_gpus
);

/**
 * Estimate memory usage for distributed inference
 */
typedef struct {
    size_t memory_per_gpu;
    size_t activation_memory;
    size_t weight_memory;
    size_t communication_overhead;
} NovaDistributedMemoryEstimate;

NovaDistributedMemoryEstimate nova_dist_estimate_memory(
    int num_layers,
    int64_t hidden_size,
    int64_t sequence_length,
    const NovaDistributedConfig *config
);

/**
 * Print distributed configuration
 */
void nova_dist_print_config(const NovaDistributedConfig *config);

#endif // NOVA_DISTRIBUTED_H
