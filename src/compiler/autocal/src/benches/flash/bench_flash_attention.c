/**
 * @file bench_flash_attention.c
 * @brief Flash Attention benchmark for autocal
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FLASH_WARMUP_ITERS 5
#define FLASH_BENCH_ITERS 20

// Simplified flash attention kernel (CPU baseline)
static void flash_attention_cpu(const float *Q, const float *K, const float *V,
                                 float *Out, int seq_len, int d_model)
{
    // Simplified: Out[i] = softmax(Q[i] @ K^T) @ V
    for (int i = 0; i < seq_len; i++) {
        float max_score = -INFINITY;
        
        // Compute attention scores
        float scores[512]; // Max seq_len = 512 for benchmark
        for (int j = 0; j < seq_len; j++) {
            float score = 0.0f;
            for (int k = 0; k < d_model; k++) {
                score += Q[i * d_model + k] * K[j * d_model + k];
            }
            scores[j] = score / sqrtf((float)d_model);
            if (scores[j] > max_score) max_score = scores[j];
        }
        
        // Softmax
        float sum_exp = 0.0f;
        for (int j = 0; j < seq_len; j++) {
            scores[j] = expf(scores[j] - max_score);
            sum_exp += scores[j];
        }
        
        // Output = weighted sum of values
        for (int k = 0; k < d_model; k++) {
            float sum = 0.0f;
            for (int j = 0; j < seq_len; j++) {
                sum += (scores[j] / sum_exp) * V[j * d_model + k];
            }
            Out[i * d_model + k] = sum;
        }
    }
}

double bench_flash_attention_latency(int seq_len, int d_model)
{
    size_t qkv_size = seq_len * d_model;
    float *Q = malloc(qkv_size * sizeof(float));
    float *K = malloc(qkv_size * sizeof(float));
    float *V = malloc(qkv_size * sizeof(float));
    float *Out = malloc(qkv_size * sizeof(float));
    
    if (!Q || !K || !V || !Out) {
        free(Q); free(K); free(V); free(Out);
        return -1.0;
    }
    
    // Initialize with random data
    for (size_t i = 0; i < qkv_size; i++) {
        Q[i] = (float)rand() / RAND_MAX;
        K[i] = (float)rand() / RAND_MAX;
        V[i] = (float)rand() / RAND_MAX;
    }
    
    // Warmup
    for (int i = 0; i < FLASH_WARMUP_ITERS; i++) {
        flash_attention_cpu(Q, K, V, Out, seq_len, d_model);
    }
    
    // Benchmark
    double start = zenith_timer_get_sec();
    for (int i = 0; i < FLASH_BENCH_ITERS; i++) {
        flash_attention_cpu(Q, K, V, Out, seq_len, d_model);
    }
    double end = zenith_timer_get_sec();
    
    free(Q); free(K); free(V); free(Out);
    
    return (end - start) / FLASH_BENCH_ITERS; // avg time per iteration
}

double bench_flash_attention_throughput(int seq_len, int d_model)
{
    double latency = bench_flash_attention_latency(seq_len, d_model);
    if (latency < 0) return 0.0;
    
    // Calculate FLOPS: 2 * seq_len^2 * d_model (simplified)
    double flops = 2.0 * seq_len * seq_len * d_model;
    return flops / latency / 1e9; // GFLOPS
}

void autocal_benchmark_flash_attention(NovaAutocalConfig *config)
{
    printf("📊 [FLASH] Benchmarking Flash Attention...\n");
    
    // Test different sizes
    struct {
        int seq_len;
        int d_model;
        const char *name;
    } configs[] = {
        {128, 64, "Small (128x64)"},
        {256, 128, "Medium (256x128)"},
        {512, 256, "Large (512x256)"}
    };
    
    double total_gflops = 0.0;
    for (int i = 0; i < 3; i++) {
        double gflops = bench_flash_attention_throughput(configs[i].seq_len, 
                                                          configs[i].d_model);
        printf("   %s: %.2f GFLOPS\n", configs[i].name, gflops);
        total_gflops += gflops;
    }
    
    // Store average performance
    config->cpu_gflops = total_gflops / 3.0;
    printf("✅ [FLASH] Average: %.2f GFLOPS\n", config->cpu_gflops);
}
