/**
 * Flash Attention Benchmark for Auto-Calibration
 * Measures Flash Attention v2 performance across different configurations
 */

#include "nova_autocal.h"
#include "nova_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Flash Attention Parameters
typedef struct {
    size_t batch_size;
    size_t seq_len;
    size_t num_heads;
    size_t head_dim;
    size_t block_size;
    bool use_causal_mask;
} FlashAttnConfig;

// Benchmark result
typedef struct {
    double time_ms;
    double gflops;
    double memory_bandwidth_gb_s;
    size_t memory_bytes;
} FlashAttnResult;

// Mock Flash Attention kernel (replace with actual implementation)
static void flash_attention_kernel(
    const float *Q, const float *K, const float *V,
    float *O, const FlashAttnConfig *config)
{
    size_t total_elements = config->batch_size * config->num_heads * 
                           config->seq_len * config->head_dim;
    
    // Simulate computation
    for (size_t i = 0; i < total_elements; i++) {
        O[i] = Q[i] * K[i] + V[i];
    }
}

// Run Flash Attention benchmark
static FlashAttnResult bench_flash_attention(const FlashAttnConfig *config, size_t iterations) {
    FlashAttnResult result = {0};
    
    // Allocate tensors
    size_t qkv_size = config->batch_size * config->num_heads * 
                      config->seq_len * config->head_dim;
    
    float *Q = (float*)malloc(qkv_size * sizeof(float));
    float *K = (float*)malloc(qkv_size * sizeof(float));
    float *V = (float*)malloc(qkv_size * sizeof(float));
    float *O = (float*)malloc(qkv_size * sizeof(float));
    
    if (!Q || !K || !V || !O) {
        fprintf(stderr, "Failed to allocate memory for Flash Attention\n");
        free(Q); free(K); free(V); free(O);
        return result;
    }
    
    // Initialize with random data
    for (size_t i = 0; i < qkv_size; i++) {
        Q[i] = (float)rand() / RAND_MAX;
        K[i] = (float)rand() / RAND_MAX;
        V[i] = (float)rand() / RAND_MAX;
    }
    
    // Warm-up
    flash_attention_kernel(Q, K, V, O, config);
    
    // Benchmark
    nova_timer_t *timer = nova_timer_create();
    nova_timer_start(timer);
    
    for (size_t i = 0; i < iterations; i++) {
        flash_attention_kernel(Q, K, V, O, config);
    }
    
    nova_timer_stop(timer);
    double total_time = nova_timer_elapsed_ms(timer);
    result.time_ms = total_time / iterations;
    
    // Calculate FLOPs
    // Flash Attention: 4 * B * H * S^2 * D operations (approximate)
    double flops = 4.0 * config->batch_size * config->num_heads * 
                   config->seq_len * config->seq_len * config->head_dim;
    result.gflops = (flops / 1e9) / (result.time_ms / 1000.0);
    
    // Calculate memory bandwidth
    result.memory_bytes = 4 * qkv_size * sizeof(float);  // Read Q, K, V, Write O
    result.memory_bandwidth_gb_s = (result.memory_bytes / 1e9) / (result.time_ms / 1000.0);
    
    // Cleanup
    nova_timer_destroy(timer);
    free(Q); free(K); free(V); free(O);
    
    return result;
}

// Auto-calibration entry point
void nova_autocal_bench_flash_attention(nova_autocal_context_t *ctx) {
    printf("=== Flash Attention Auto-Calibration ===\n\n");
    
    // Test configurations
    FlashAttnConfig configs[] = {
        // Small: GPT-2 style
        {.batch_size = 8, .seq_len = 512, .num_heads = 12, .head_dim = 64, .block_size = 64, .use_causal_mask = true},
        // Medium: BERT style
        {.batch_size = 16, .seq_len = 512, .num_heads = 12, .head_dim = 64, .block_size = 64, .use_causal_mask = false},
        // Large: GPT-3 style
        {.batch_size = 4, .seq_len = 2048, .num_heads = 32, .head_dim = 128, .block_size = 128, .use_causal_mask = true},
        // XL: Long context
        {.batch_size = 2, .seq_len = 4096, .num_heads = 16, .head_dim = 128, .block_size = 256, .use_causal_mask = false},
    };
    
    size_t num_configs = sizeof(configs) / sizeof(configs[0]);
    size_t iterations = 100;
    
    for (size_t i = 0; i < num_configs; i++) {
        FlashAttnConfig *cfg = &configs[i];
        
        printf("Config %zu: B=%zu, S=%zu, H=%zu, D=%zu, Block=%zu, Causal=%s\n",
               i + 1, cfg->batch_size, cfg->seq_len, cfg->num_heads, cfg->head_dim,
               cfg->block_size, cfg->use_causal_mask ? "Yes" : "No");
        
        FlashAttnResult res = bench_flash_attention(cfg, iterations);
        
        printf("  Time: %.3f ms\n", res.time_ms);
        printf("  Throughput: %.2f GFLOPS\n", res.gflops);
        printf("  Memory BW: %.2f GB/s\n", res.memory_bandwidth_gb_s);
        printf("  Memory: %.2f MB\n\n", res.memory_bytes / 1e6);
        
        // Record to calibration context
        char key[256];
        snprintf(key, sizeof(key), "flash_attn_b%zu_s%zu_h%zu_d%zu", 
                 cfg->batch_size, cfg->seq_len, cfg->num_heads, cfg->head_dim);
        nova_autocal_record_metric(ctx, key, "time_ms", res.time_ms);
        nova_autocal_record_metric(ctx, key, "gflops", res.gflops);
        nova_autocal_record_metric(ctx, key, "memory_bw_gb_s", res.memory_bandwidth_gb_s);
    }
    
    printf("Flash Attention calibration complete.\n");
}
