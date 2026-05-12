#ifndef NOVA_ADVANCED_OPTIMIZATIONS_H
#define NOVA_ADVANCED_OPTIMIZATIONS_H

#include "nova_arena.h"
#include "nova_fp16_kernels.h"
#include "nova_memory_arena.h"

// Forward declarations to break circularity
typedef struct CLIPConditioner CLIPConditioner;
typedef struct StableDiffusionUNet StableDiffusionUNet;
typedef struct VAEDecoder VAEDecoder;
typedef struct NovaTensor NovaTensor;

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * KERNEL FUSION (CRITICAL PRODUCTION OPTIMIZATION)
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  NovaComputeContext *ctx;
  NovaNumericsPolicy numerics;

  void *fused_attention_kernel;
  void *fused_matmul_bias_kernel;
  void *fused_conv_norm_kernel;

  void **kernel_cache;
  int cache_size;
} NovaKernelFusion;

NovaKernelFusion *nova_kernel_fusion_create(NovaComputeContext *ctx);
void nova_kernel_fusion_free(NovaKernelFusion *fusion);

int nova_fused_attention(NovaKernelFusion *fusion, const void *Q,
                           const void *K, const void *V, const void *bias,
                           void *output, int seq_len, int embed_dim,
                           int num_heads);

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * FLASH ATTENTION
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  NovaComputeContext *ctx;
  NovaNumericsPolicy numerics;
  int block_size;
  int num_blocks;
  bool causal;
  void *scratch_q;
  void *scratch_k;
  void *scratch_v;
  void *scratch_attn;
  size_t scratch_size;
} NovaFlashAttention;

NovaFlashAttention *nova_flash_attention_create(NovaComputeContext *ctx,
                                                    int block_size);
void nova_flash_attention_free(NovaFlashAttention *flash);
int nova_flash_attention_forward(NovaFlashAttention *flash, const void *Q,
                                   const void *K, const void *V, void *output,
                                   int seq_len, int embed_dim, int num_heads);
int nova_flash_attention_backward(NovaFlashAttention *flash,
                                    const void *grad_output, const void *Q,
                                    const void *K, const void *V, void *grad_q,
                                    void *grad_k, void *grad_v, int seq_len,
                                    int embed_dim, int num_heads);

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * TENSOR MEMORY POOLING
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct NovaTensorPool NovaTensorPool;

typedef struct {
  int ndim;
  int *shape;
  NovaDType dtype;
  NovaLifetimeClass lifetime;
  char debug_name[64];
} NovaTensorSpec;

typedef struct {
  size_t total_allocations;
  size_t pool_hits;
  size_t pool_misses;
  double hit_ratio;
  size_t peak_memory_usage;
} NovaTensorPoolStats;

typedef struct {
  NovaComputeContext *ctx;
  NovaNumericsPolicy numerics;
  float current_scale;
  float max_scale;
  float min_scale;
  float scale_factor;
  float max_grad_norm;
  bool clip_gradients;
  int overflow_steps;
  int underflow_steps;
  float avg_scale;
} NovaLossScaler;

NovaTensorPool *nova_tensor_pool_create(NovaMemoryArena *arena,
                                            size_t max_pool_size);
void nova_tensor_pool_free(NovaTensorPool *pool);
void nova_tensor_pool_get_stats(NovaTensorPool *pool,
                                  NovaTensorPoolStats *stats);

NovaLossScaler *nova_loss_scaler_create(NovaComputeContext *ctx,
                                            float initial_scale);
void nova_loss_scaler_free(NovaLossScaler *scaler);
float nova_loss_scaler_scale_loss(NovaLossScaler *scaler, float loss);
bool nova_loss_scaler_unscale_gradients(NovaLossScaler *scaler,
                                          void *gradients, int num_elements);
void nova_loss_scaler_update_scale(NovaLossScaler *scaler,
                                     bool has_overflow);

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * INTEGRATION: OPTIMIZED SD PIPELINE
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  CLIPConditioner *text_encoder;
  StableDiffusionUNet *unet;
  VAEDecoder *vae_decoder;

  NovaKernelFusion *kernel_fusion;
  NovaFlashAttention *flash_attention;
  NovaTensorPool *tensor_pool;

  double kernel_fusion_time_saved;
  double flash_attention_memory_saved;
} NovaSDPipelineOptimized;

typedef struct {
  NovaComputeContext *ctx;
  NovaMemoryArena *arena;
  int num_layers;
  int max_seq_len;
  int embed_dim;
  int current_seq_len;
  void **key_cache;
  void **value_cache;
} NovaKVCache;

NovaKVCache *nova_kv_cache_create(NovaComputeContext *ctx,
                                      NovaMemoryArena *arena, int num_layers,
                                      int max_seq_len, int embed_dim);
typedef struct {
  size_t total_keys;
  size_t total_values;
  size_t bytes_used;
  int current_seq_len;
  size_t cache_memory_usage;
  int max_sequence_length;
  int cache_hits;
  int cache_misses;
  double hit_ratio;
} NovaKVCacheStats;

void nova_kv_cache_get_stats(NovaKVCache *cache, NovaKVCacheStats *stats);

void nova_kv_cache_free(NovaKVCache *cache);

typedef struct {
  Arena *arena;
  int num_nodes;
  void **nodes;
  int *dependencies;
  bool *is_required;
} NovaGraphPruner;

NovaGraphPruner *nova_graph_pruner_create(Arena *arena);
void nova_graph_pruner_free(NovaGraphPruner *pruner);
int nova_graph_prune(NovaGraphPruner *pruner, void *output_node);
bool nova_graph_is_node_required(NovaGraphPruner *pruner, int node_id);

NovaSDPipelineOptimized *
nova_sd_pipeline_optimized_create(NovaBackendType backend);

#endif // NOVA_ADVANCED_OPTIMIZATIONS_H
