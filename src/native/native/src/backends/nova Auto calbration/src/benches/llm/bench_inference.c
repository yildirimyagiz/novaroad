/**
 * LLM Inference Benchmark for Auto-Calibration
 * Tests transformer inference performance and token throughput
 */

#include "nova_autocal.h"
#include "nova_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t batch_size;
    size_t seq_len;
    size_t vocab_size;
    size_t hidden_size;
    size_t num_layers;
    size_t num_heads;
} LLMConfig;

typedef struct {
    double time_per_token_ms;
    double tokens_per_sec;
    double throughput_mb_s;
    size_t memory_bytes;
} LLMResult;

// Simplified transformer layer
static void transformer_layer(float *hidden, const LLMConfig *cfg) {
    size_t hidden_total = cfg->batch_size * cfg->seq_len * cfg->hidden_size;
    
    // Self-attention (simplified)
    for (size_t i = 0; i < hidden_total; i++) {
        hidden[i] = hidden[i] * 0.5f + 0.1f;
    }
    
    // FFN (simplified)
    for (size_t i = 0; i < hidden_total; i++) {
        float tmp = hidden[i] * 4.0f;  // Expand
        tmp = (tmp > 0) ? tmp : 0;      // GELU approximation
        hidden[i] = tmp * 0.25f;        // Project back
    }
}

// Full model inference
static void llm_forward(float *input, float *output, const LLMConfig *cfg) {
    size_t hidden_size = cfg->batch_size * cfg->seq_len * cfg->hidden_size;
    
    // Embedding (simplified)
    memcpy(output, input, hidden_size * sizeof(float));
    
    // Transformer layers
    for (size_t layer = 0; layer < cfg->num_layers; layer++) {
        transformer_layer(output, cfg);
    }
    
    // Final projection to vocab (simplified)
    for (size_t i = 0; i < cfg->batch_size * cfg->seq_len; i++) {
        size_t offset = i * cfg->hidden_size;
        output[i] = output[offset];  // Pick first hidden dim
    }
}

static LLMResult bench_llm_inference(const LLMConfig *cfg, size_t iterations) {
    LLMResult result = {0};
    
    size_t input_size = cfg->batch_size * cfg->seq_len * cfg->hidden_size;
    size_t output_size = cfg->batch_size * cfg->seq_len;
    
    float *input = (float*)malloc(input_size * sizeof(float));
    float *output = (float*)malloc(output_size * sizeof(float));
    
    if (!input || !output) {
        fprintf(stderr, "Failed to allocate memory for LLM inference\n");
        free(input); free(output);
        return result;
    }
    
    // Initialize input
    for (size_t i = 0; i < input_size; i++) {
        input[i] = (float)(rand() % 1000) / 1000.0f;
    }
    
    // Warm-up
    llm_forward(input, output, cfg);
    
    // Benchmark
    nova_timer_t *timer = nova_timer_create();
    nova_timer_start(timer);
    
    for (size_t i = 0; i < iterations; i++) {
        llm_forward(input, output, cfg);
    }
    
    nova_timer_stop(timer);
    double total_time = nova_timer_elapsed_ms(timer);
    
    // Calculate metrics
    size_t total_tokens = cfg->batch_size * cfg->seq_len * iterations;
    result.time_per_token_ms = total_time / total_tokens;
    result.tokens_per_sec = (total_tokens * 1000.0) / total_time;
    
    result.memory_bytes = (input_size + output_size) * sizeof(float);
    result.throughput_mb_s = (result.memory_bytes / 1e6) * result.tokens_per_sec / 
                             (cfg->batch_size * cfg->seq_len);
    
    nova_timer_destroy(timer);
    free(input); free(output);
    
    return result;
}

void nova_autocal_bench_llm_inference(nova_autocal_context_t *ctx) {
    printf("=== LLM Inference Auto-Calibration ===\n\n");
    
    LLMConfig configs[] = {
        // GPT-2 Small: 124M params
        {.batch_size = 4, .seq_len = 512, .vocab_size = 50257, 
         .hidden_size = 768, .num_layers = 12, .num_heads = 12},
        
        // GPT-2 Medium: 350M params
        {.batch_size = 2, .seq_len = 1024, .vocab_size = 50257,
         .hidden_size = 1024, .num_layers = 24, .num_heads = 16},
        
        // GPT-2 Large: 774M params
        {.batch_size = 1, .seq_len = 1024, .vocab_size = 50257,
         .hidden_size = 1280, .num_layers = 36, .num_heads = 20},
        
        // Small batch, long context
        {.batch_size = 1, .seq_len = 2048, .vocab_size = 50257,
         .hidden_size = 768, .num_layers = 12, .num_heads = 12},
    };
    
    size_t num_configs = sizeof(configs) / sizeof(configs[0]);
    
    for (size_t i = 0; i < num_configs; i++) {
        LLMConfig *cfg = &configs[i];
        
        printf("Config %zu: B=%zu, S=%zu, H=%zu, L=%zu\n",
               i + 1, cfg->batch_size, cfg->seq_len, cfg->hidden_size, cfg->num_layers);
        
        LLMResult res = bench_llm_inference(cfg, 10);
        
        printf("  Time/token: %.3f ms\n", res.time_per_token_ms);
        printf("  Tokens/sec: %.2f\n", res.tokens_per_sec);
        printf("  Throughput: %.2f MB/s\n", res.throughput_mb_s);
        printf("  Memory: %.2f MB\n\n", res.memory_bytes / 1e6);
        
        char key[256];
        snprintf(key, sizeof(key), "llm_b%zu_s%zu_h%zu_l%zu",
                 cfg->batch_size, cfg->seq_len, cfg->hidden_size, cfg->num_layers);
        nova_autocal_record_metric(ctx, key, "time_per_token_ms", res.time_per_token_ms);
        nova_autocal_record_metric(ctx, key, "tokens_per_sec", res.tokens_per_sec);
    }
    
    printf("LLM inference calibration complete.\n");
}
