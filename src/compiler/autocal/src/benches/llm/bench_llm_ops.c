/**
 * @file bench_llm_ops.c
 * @brief LLM-specific operation benchmarks
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define LLM_BENCH_ITERS 50

// Layer normalization
static void layer_norm(const float *input, float *output, int batch, int hidden_dim)
{
    for (int b = 0; b < batch; b++) {
        const float *x = input + b * hidden_dim;
        float *y = output + b * hidden_dim;
        
        // Compute mean
        float mean = 0.0f;
        for (int i = 0; i < hidden_dim; i++) {
            mean += x[i];
        }
        mean /= hidden_dim;
        
        // Compute variance
        float var = 0.0f;
        for (int i = 0; i < hidden_dim; i++) {
            float diff = x[i] - mean;
            var += diff * diff;
        }
        var /= hidden_dim;
        
        // Normalize
        float inv_std = 1.0f / sqrtf(var + 1e-5f);
        for (int i = 0; i < hidden_dim; i++) {
            y[i] = (x[i] - mean) * inv_std;
        }
    }
}

// GELU activation (approximate)
static void gelu(const float *input, float *output, int n)
{
    for (int i = 0; i < n; i++) {
        float x = input[i];
        // GELU(x) ≈ 0.5 * x * (1 + tanh(sqrt(2/π) * (x + 0.044715 * x^3)))
        float x3 = x * x * x;
        float inner = 0.7978845608f * (x + 0.044715f * x3);
        output[i] = 0.5f * x * (1.0f + tanhf(inner));
    }
}

// Embedding lookup
static void embedding_lookup(const int *indices, const float *embedding_table,
                              float *output, int batch, int seq_len, int embed_dim, int vocab_size)
{
    for (int b = 0; b < batch; b++) {
        for (int s = 0; s < seq_len; s++) {
            int idx = indices[b * seq_len + s];
            if (idx >= 0 && idx < vocab_size) {
                const float *emb = embedding_table + idx * embed_dim;
                float *out = output + (b * seq_len + s) * embed_dim;
                for (int d = 0; d < embed_dim; d++) {
                    out[d] = emb[d];
                }
            }
        }
    }
}

double bench_llm_layernorm(int batch, int hidden_dim)
{
    size_t size = batch * hidden_dim;
    float *input = malloc(size * sizeof(float));
    float *output = malloc(size * sizeof(float));
    
    if (!input || !output) {
        free(input); free(output);
        return -1.0;
    }
    
    for (size_t i = 0; i < size; i++) {
        input[i] = (float)rand() / RAND_MAX;
    }
    
    double start = zenith_timer_get_sec();
    for (int i = 0; i < LLM_BENCH_ITERS; i++) {
        layer_norm(input, output, batch, hidden_dim);
    }
    double end = zenith_timer_get_sec();
    
    free(input); free(output);
    return (end - start) / LLM_BENCH_ITERS;
}

void autocal_benchmark_llm_ops(NovaAutocalConfig *config)
{
    printf("📊 [LLM] Benchmarking LLM Operations...\n");
    
    // Test LayerNorm (common bottleneck)
    struct {
        int batch;
        int hidden;
        const char *name;
    } configs[] = {
        {1, 768, "BERT-base"},
        {1, 1024, "GPT-2 Medium"},
        {1, 2048, "GPT-3 Small"}
    };
    
    double total_time = 0.0;
    for (int i = 0; i < 3; i++) {
        double latency = bench_llm_layernorm(configs[i].batch, configs[i].hidden);
        printf("   %s (hidden=%d): %.3f ms\n", configs[i].name, 
               configs[i].hidden, latency * 1000);
        total_time += latency;
    }
    
    printf("✅ [LLM] Average LayerNorm latency: %.3f ms\n", (total_time / 3.0) * 1000);
}
