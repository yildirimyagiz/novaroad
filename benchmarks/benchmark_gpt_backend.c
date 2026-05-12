/**
 * benchmark_gpt_backend.c - GPT Backend Performance Benchmark
 * 
 * Measures real-world performance of GPT operations across backends
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

// Timing utilities
static double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

// Benchmark configuration
typedef struct {
    int64_t batch_size;
    int64_t seq_len;
    int64_t hidden_size;
    int64_t num_heads;
    int num_iterations;
} BenchConfig;

// Benchmark Flash Attention
void benchmark_flash_attention(const BenchConfig *cfg) {
    printf("\n🔬 Benchmarking Flash Attention\n");
    printf("   Config: batch=%lld, seq_len=%lld, hidden=%lld, heads=%lld\n",
           cfg->batch_size, cfg->seq_len, cfg->hidden_size, cfg->num_heads);
    
    int64_t head_dim = cfg->hidden_size / cfg->num_heads;
    int64_t shape[4] = {cfg->batch_size, cfg->num_heads, cfg->seq_len, head_dim};
    
    // Create tensors
    NovaTensor *Q = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *K = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *V = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *output = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    
    // Initialize with random data
    float *q_data = (float *)Q->data;
    float *k_data = (float *)K->data;
    float *v_data = (float *)V->data;
    
    for (size_t i = 0; i < Q->total_elements; i++) {
        q_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        k_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        v_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
    }
    
    // Warmup
    float scale = 1.0f / sqrtf((float)head_dim);
    nova_gpt_flash_attention(Q, K, V, output, true, scale);
    
    // Benchmark
    double total_time = 0.0;
    double min_time = 1e9;
    double max_time = 0.0;
    
    for (int i = 0; i < cfg->num_iterations; i++) {
        double start = get_time_ms();
        nova_gpt_flash_attention(Q, K, V, output, true, scale);
        double end = get_time_ms();
        
        double elapsed = end - start;
        total_time += elapsed;
        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;
    }
    
    double avg_time = total_time / cfg->num_iterations;
    
    // Calculate FLOPs
    // Attention: 2 * N^2 * D (QK^T) + 2 * N^2 * D (softmax*V)
    int64_t N = cfg->seq_len;
    int64_t D = head_dim;
    int64_t H = cfg->num_heads;
    int64_t B = cfg->batch_size;
    
    double flops = (double)B * H * (4.0 * N * N * D);
    double gflops = flops / (avg_time / 1000.0) / 1e9;
    
    printf("   Results:\n");
    printf("     Average time:  %.2f ms\n", avg_time);
    printf("     Min time:      %.2f ms\n", min_time);
    printf("     Max time:      %.2f ms\n", max_time);
    printf("     Throughput:    %.2f GFLOPS\n", gflops);
    printf("     Tokens/sec:    %.0f\n", (cfg->seq_len * 1000.0) / avg_time);
    
    // Cleanup
    nova_tensor_destroy(Q);
    nova_tensor_destroy(K);
    nova_tensor_destroy(V);
    nova_tensor_destroy(output);
}

// Benchmark KV Cache
void benchmark_kv_cache(const BenchConfig *cfg) {
    printf("\n🔬 Benchmarking KV Cache\n");
    
    NovaGPTConfig config = {
        .vocab_size = 32000,
        .hidden_size = cfg->hidden_size,
        .num_hidden_layers = 4,
        .num_attention_heads = cfg->num_heads,
        .num_key_value_heads = cfg->num_heads,
        .max_position_embeddings = 2048,
        .use_cache = true
    };
    
    double start = get_time_ms();
    NovaKVCache *cache = nova_kv_cache_create(
        NULL, &config, cfg->batch_size, NOVA_DEVICE_CPU
    );
    double create_time = get_time_ms() - start;
    
    printf("   Cache creation: %.2f ms\n", create_time);
    
    // Simulate cache updates
    int64_t head_dim = cfg->hidden_size / cfg->num_heads;
    int64_t kv_shape[4] = {
        cfg->batch_size,
        config.num_key_value_heads,
        cfg->seq_len,
        head_dim
    };
    
    NovaTensor *keys = nova_tensor_create(NULL, kv_shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *values = nova_tensor_create(NULL, kv_shape, 4, NOVA_DTYPE_FP32);
    
    start = get_time_ms();
    for (int i = 0; i < config.num_hidden_layers; i++) {
        nova_kv_cache_update(cache, i, keys, values);
    }
    double update_time = get_time_ms() - start;
    
    printf("   Cache update:   %.2f ms\n", update_time);
    
    // Clear cache
    start = get_time_ms();
    nova_kv_cache_clear(cache);
    double clear_time = get_time_ms() - start;
    
    printf("   Cache clear:    %.2f ms\n", clear_time);
    
    // Cleanup
    nova_tensor_destroy(keys);
    nova_tensor_destroy(values);
    nova_kv_cache_destroy(cache);
}

// Benchmark RMSNorm
void benchmark_rms_norm(const BenchConfig *cfg) {
    printf("\n🔬 Benchmarking RMSNorm\n");
    
    int64_t shape[2] = {cfg->batch_size * cfg->seq_len, cfg->hidden_size};
    
    NovaTensor *input = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *weight = nova_tensor_ones(NULL, &shape[1], 1);
    NovaTensor *output = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    // Initialize
    float *data = (float *)input->data;
    for (size_t i = 0; i < input->total_elements; i++) {
        data[i] = ((float)rand() / RAND_MAX) - 0.5f;
    }
    
    // Benchmark
    double total_time = 0.0;
    for (int i = 0; i < cfg->num_iterations; i++) {
        double start = get_time_ms();
        nova_gpt_rms_norm(input, weight, output, 1e-6f);
        double end = get_time_ms();
        total_time += (end - start);
    }
    
    double avg_time = total_time / cfg->num_iterations;
    printf("   Average time: %.2f ms\n", avg_time);
    
    // Cleanup
    nova_tensor_destroy(input);
    nova_tensor_destroy(weight);
    nova_tensor_destroy(output);
}

// Main benchmark suite
int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("   NOVA GPT BACKEND PERFORMANCE BENCHMARK\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    // Initialize backend
    nova_backend_init(NOVA_BACKEND_AUTO);
    NovaBackendStatus status = nova_backend_status();
    
    printf("\n🖥️  Backend: %s\n", nova_backend_name(status.active));
    printf("   CUDA:   %s\n", status.cuda_available ? "✅" : "❌");
    printf("   Metal:  %s\n", status.metal_available ? "✅" : "❌");
    printf("   Vulkan: %s\n", status.vulkan_available ? "✅" : "❌");
    
    // Benchmark configurations
    BenchConfig configs[] = {
        // Small model (testing)
        {.batch_size = 1, .seq_len = 128, .hidden_size = 512, .num_heads = 8, .num_iterations = 10},
        // Medium model
        {.batch_size = 1, .seq_len = 512, .hidden_size = 1024, .num_heads = 16, .num_iterations = 5},
        // Large model
        {.batch_size = 1, .seq_len = 1024, .hidden_size = 2048, .num_heads = 32, .num_iterations = 3},
    };
    
    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
        printf("\n═══════════════════════════════════════════════════════════════\n");
        printf("   Configuration %zu\n", i + 1);
        printf("═══════════════════════════════════════════════════════════════\n");
        
        benchmark_flash_attention(&configs[i]);
        benchmark_kv_cache(&configs[i]);
        benchmark_rms_norm(&configs[i]);
    }
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("   ✅ BENCHMARK COMPLETE\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    nova_backend_cleanup();
    return 0;
}
