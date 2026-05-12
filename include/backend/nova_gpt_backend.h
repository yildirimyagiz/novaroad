/**
 * nova_gpt_backend.h - GPT Model Backend Integration
 * 
 * Connects Nova GPT models to the unified backend dispatcher
 * Provides high-level GPT operations with automatic backend selection
 */

#ifndef NOVA_GPT_BACKEND_H
#define NOVA_GPT_BACKEND_H

#include "../ml/nova_tensor.h"
#include "nova_backend_dispatch.h"
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// GPT Model Configuration
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int64_t vocab_size;
    int64_t hidden_size;
    int64_t num_hidden_layers;
    int64_t num_attention_heads;
    int64_t num_key_value_heads;  // For Grouped Query Attention
    int64_t intermediate_size;
    int64_t max_position_embeddings;
    int64_t sliding_window;       // For sliding window attention
    float rope_theta;
    float rms_norm_eps;
    bool use_cache;               // Enable KV caching
    bool use_flash_attention;     // Use Flash Attention-2
} NovaGPTConfig;

// ═══════════════════════════════════════════════════════════════════════════
// KV Cache Management
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    NovaTensor **keys;    // Array of key tensors per layer
    NovaTensor **values;  // Array of value tensors per layer
    int64_t num_layers;
    int64_t max_seq_len;
    int64_t current_seq_len;
    NovaDevice device;
} NovaKVCache;

// Create and destroy KV cache
NovaKVCache *nova_kv_cache_create(
    NovaContext *ctx,
    const NovaGPTConfig *config,
    int64_t batch_size,
    NovaDevice device
);

void nova_kv_cache_destroy(NovaKVCache *cache);

// Update KV cache
int nova_kv_cache_update(
    NovaKVCache *cache,
    int layer_idx,
    NovaTensor *new_keys,
    NovaTensor *new_values
);

// Clear cache
void nova_kv_cache_clear(NovaKVCache *cache);

// ═══════════════════════════════════════════════════════════════════════════
// Attention Operations (Backend-Dispatched)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Flash Attention-2 with automatic backend selection
 * Dispatches to: CUDA > Metal > CPU optimized implementation
 * 
 * @param Q Query tensor [batch, num_heads, seq_len, head_dim]
 * @param K Key tensor [batch, num_heads, seq_len, head_dim]
 * @param V Value tensor [batch, num_heads, seq_len, head_dim]
 * @param output Output tensor (pre-allocated)
 * @param causal Apply causal masking for autoregressive generation
 * @param scale Attention scale factor (typically 1/sqrt(head_dim))
 * @return 0 on success, negative on error
 */
int nova_gpt_flash_attention(
    const NovaTensor *Q,
    const NovaTensor *K,
    const NovaTensor *V,
    NovaTensor *output,
    bool causal,
    float scale
);

/**
 * Multi-Head Attention with KV cache support
 */
int nova_gpt_multi_head_attention(
    const NovaTensor *query,
    const NovaTensor *Wq,      // Query projection weight
    const NovaTensor *Wk,      // Key projection weight
    const NovaTensor *Wv,      // Value projection weight
    const NovaTensor *Wo,      // Output projection weight
    NovaKVCache *kv_cache,
    int layer_idx,
    NovaTensor *output,
    const NovaGPTConfig *config
);

/**
 * Grouped Query Attention (for efficiency)
 * Used in Mistral, LLaMA-2, etc.
 */
int nova_gpt_grouped_query_attention(
    const NovaTensor *query,
    const NovaTensor *key,
    const NovaTensor *value,
    NovaTensor *output,
    int64_t num_q_heads,
    int64_t num_kv_heads,
    bool causal,
    float scale
);

// ═══════════════════════════════════════════════════════════════════════════
// Rotary Position Embeddings (RoPE)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    NovaTensor *cos_cache;
    NovaTensor *sin_cache;
    int64_t max_seq_len;
    int64_t head_dim;
    float theta;
} NovaRoPECache;

NovaRoPECache *nova_rope_cache_create(
    NovaContext *ctx,
    int64_t max_seq_len,
    int64_t head_dim,
    float theta,
    NovaDevice device
);

void nova_rope_cache_destroy(NovaRoPECache *cache);

/**
 * Apply rotary position embeddings
 * Supports both standard RoPE and NTK-aware scaling
 */
int nova_gpt_apply_rope(
    NovaTensor *tensor,
    const NovaRoPECache *cache,
    int64_t position_offset
);

// ═══════════════════════════════════════════════════════════════════════════
// Feed-Forward Networks
// ═══════════════════════════════════════════════════════════════════════════

/**
 * SwiGLU activation (used in LLaMA, Mistral)
 * out = swish(W1 @ x) * (W2 @ x)
 */
int nova_gpt_swiglu_ffn(
    const NovaTensor *input,
    const NovaTensor *W1,
    const NovaTensor *W2,
    const NovaTensor *W3,
    NovaTensor *output
);

/**
 * GELU activation FFN (used in GPT-2, GPT-3)
 */
int nova_gpt_gelu_ffn(
    const NovaTensor *input,
    const NovaTensor *W1,
    const NovaTensor *W2,
    const NovaTensor *b1,
    const NovaTensor *b2,
    NovaTensor *output
);

// ═══════════════════════════════════════════════════════════════════════════
// Normalization
// ═══════════════════════════════════════════════════════════════════════════

/**
 * RMSNorm (Root Mean Square Layer Normalization)
 * Used in LLaMA, Mistral
 */
int nova_gpt_rms_norm(
    const NovaTensor *input,
    const NovaTensor *weight,
    NovaTensor *output,
    float eps
);

/**
 * LayerNorm (used in GPT-2, GPT-3)
 */
int nova_gpt_layer_norm(
    const NovaTensor *input,
    const NovaTensor *weight,
    const NovaTensor *bias,
    NovaTensor *output,
    float eps
);

// ═══════════════════════════════════════════════════════════════════════════
// Sampling & Generation
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_SAMPLE_GREEDY,
    NOVA_SAMPLE_TOP_K,
    NOVA_SAMPLE_TOP_P,
    NOVA_SAMPLE_TEMPERATURE
} NovaSamplingStrategy;

typedef struct {
    NovaSamplingStrategy strategy;
    float temperature;
    int top_k;
    float top_p;
    int64_t max_new_tokens;
    bool use_cache;
} NovaGenerationConfig;

/**
 * Sample next token from logits
 */
int64_t nova_gpt_sample_token(
    const NovaTensor *logits,
    const NovaGenerationConfig *config,
    uint64_t *rng_state
);

/**
 * Top-K sampling
 */
int64_t nova_gpt_sample_top_k(
    const NovaTensor *logits,
    int k,
    float temperature,
    uint64_t *rng_state
);

/**
 * Top-P (nucleus) sampling
 */
int64_t nova_gpt_sample_top_p(
    const NovaTensor *logits,
    float p,
    float temperature,
    uint64_t *rng_state
);

// ═══════════════════════════════════════════════════════════════════════════
// Backend Performance Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Benchmark GPT operations on current backend
 */
typedef struct {
    double attention_time_ms;
    double ffn_time_ms;
    double norm_time_ms;
    double total_time_ms;
    int64_t tokens_per_second;
    NovaBackendType backend_used;
} NovaGPTBenchmark;

NovaGPTBenchmark nova_gpt_benchmark(
    const NovaGPTConfig *config,
    int64_t num_iterations
);

/**
 * Get recommended backend for GPT inference
 */
NovaBackendType nova_gpt_get_optimal_backend(
    const NovaGPTConfig *config,
    int64_t batch_size
);

#endif // NOVA_GPT_BACKEND_H
