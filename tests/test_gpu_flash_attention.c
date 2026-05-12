/**
 * test_gpu_flash_attention.c - GPU Flash Attention Tests
 * 
 * Tests Metal and CUDA Flash Attention kernels
 */

#include "nova_gpt_backend.h"
#include "nova_tensor.h"
#include "nova_backend_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "❌ TEST FAILED: %s\n", msg); \
        return -1; \
    } else { \
        printf("✅ %s\n", msg); \
    } \
} while(0)

static double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 1: Metal Flash Attention
// ═══════════════════════════════════════════════════════════════════════════

int test_metal_flash_attention() {
    printf("\n🧪 Test: Metal Flash Attention\n");
    
#ifdef __APPLE__
    // Initialize Metal backend
    extern int nova_metal_flash_attention_init(void);
    int result = nova_metal_flash_attention_init();
    
    if (result < 0) {
        printf("⚠️  Metal not available, skipping test\n");
        return 0;  // Not a failure, just not available
    }
    
    // Small test: [batch=1, heads=2, seq_len=128, head_dim=64]
    int64_t batch = 1;
    int64_t num_heads = 2;
    int64_t seq_len = 128;
    int64_t head_dim = 64;
    
    int64_t shape[4] = {batch, num_heads, seq_len, head_dim};
    
    NovaTensor *Q = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *K = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *V = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *output = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    
    TEST_ASSERT(Q && K && V && output, "Tensors created");
    
    // Initialize with random data
    float *q_data = (float *)Q->data;
    float *k_data = (float *)K->data;
    float *v_data = (float *)V->data;
    
    for (size_t i = 0; i < Q->total_elements; i++) {
        q_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        k_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        v_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
    }
    
    // Run Metal Flash Attention
    float scale = 1.0f / sqrtf((float)head_dim);
    
    extern int64_t nova_metal_flash_attention(const float*, const float*, 
                                               const float*, float*,
                                               int64_t, int64_t, int64_t, int64_t,
                                               float, bool);
    
    double start = get_time_ms();
    result = nova_metal_flash_attention(
        q_data, k_data, v_data, (float*)output->data,
        batch, num_heads, seq_len, head_dim,
        scale, true
    );
    double elapsed = get_time_ms() - start;
    
    TEST_ASSERT(result >= 0, "Metal Flash Attention executed");
    printf("   Execution time: %.2f ms\n", elapsed);
    
    // Verify output
    float *out_data = (float *)output->data;
    bool has_nonzero = false;
    for (size_t i = 0; i < 100 && i < output->total_elements; i++) {
        if (fabsf(out_data[i]) > 1e-6f) {
            has_nonzero = true;
            break;
        }
    }
    TEST_ASSERT(has_nonzero, "Output contains non-zero values");
    
    // Cleanup
    nova_tensor_destroy(Q);
    nova_tensor_destroy(K);
    nova_tensor_destroy(V);
    nova_tensor_destroy(output);
    
    extern void nova_metal_flash_attention_cleanup(void);
    nova_metal_flash_attention_cleanup();
    
    printf("✅ Metal Flash Attention test passed\n");
#else
    printf("⚠️  Not on Apple platform, skipping Metal test\n");
#endif
    
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 2: CUDA Flash Attention
// ═══════════════════════════════════════════════════════════════════════════

int test_cuda_flash_attention() {
    printf("\n🧪 Test: CUDA Flash Attention\n");
    
#ifdef __CUDACC__
    // Test CUDA kernel
    int64_t batch = 1;
    int64_t num_heads = 2;
    int64_t seq_len = 128;
    int64_t head_dim = 64;
    
    int64_t shape[4] = {batch, num_heads, seq_len, head_dim};
    
    NovaTensor *Q = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *K = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *V = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *output = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    
    // Initialize
    float *q_data = (float *)Q->data;
    float *k_data = (float *)K->data;
    float *v_data = (float *)V->data;
    
    for (size_t i = 0; i < Q->total_elements; i++) {
        q_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        k_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        v_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
    }
    
    // Run CUDA kernel
    extern int64_t nova_cuda_flash_attention(const float*, const float*,
                                              const float*, float*,
                                              int64_t, int64_t, int64_t, int64_t,
                                              float, bool);
    
    double start = get_time_ms();
    int result = nova_cuda_flash_attention(
        q_data, k_data, v_data, (float*)output->data,
        batch, num_heads, seq_len, head_dim,
        1.0f / sqrtf((float)head_dim), true
    );
    double elapsed = get_time_ms() - start;
    
    TEST_ASSERT(result >= 0, "CUDA Flash Attention executed");
    printf("   Execution time: %.2f ms\n", elapsed);
    
    // Cleanup
    nova_tensor_destroy(Q);
    nova_tensor_destroy(K);
    nova_tensor_destroy(V);
    nova_tensor_destroy(output);
    
    printf("✅ CUDA Flash Attention test passed\n");
#else
    printf("⚠️  CUDA not available, skipping test\n");
#endif
    
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 3: Performance Comparison
// ═══════════════════════════════════════════════════════════════════════════

int test_performance_comparison() {
    printf("\n🧪 Test: Performance Comparison (CPU vs GPU)\n");
    
    int64_t batch = 1;
    int64_t num_heads = 8;
    int64_t seq_len = 512;
    int64_t head_dim = 64;
    
    int64_t shape[4] = {batch, num_heads, seq_len, head_dim};
    
    NovaTensor *Q = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *K = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *V = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *output_cpu = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *output_gpu = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    
    // Initialize
    float *q_data = (float *)Q->data;
    float *k_data = (float *)K->data;
    float *v_data = (float *)V->data;
    
    for (size_t i = 0; i < Q->total_elements; i++) {
        q_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        k_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
        v_data[i] = ((float)rand() / RAND_MAX) - 0.5f;
    }
    
    float scale = 1.0f / sqrtf((float)head_dim);
    
    // CPU benchmark
    printf("   Running CPU Flash Attention...\n");
    double cpu_start = get_time_ms();
    int cpu_result = nova_gpt_flash_attention(Q, K, V, output_cpu, true, scale);
    double cpu_time = get_time_ms() - cpu_start;
    
    printf("   CPU time: %.2f ms\n", cpu_time);
    
    // GPU benchmark (if available)
#ifdef __APPLE__
    extern int nova_metal_flash_attention_init(void);
    if (nova_metal_flash_attention_init() == 0) {
        printf("   Running Metal Flash Attention...\n");
        
        extern int64_t nova_metal_flash_attention(const float*, const float*,
                                                   const float*, float*,
                                                   int64_t, int64_t, int64_t, int64_t,
                                                   float, bool);
        
        double gpu_start = get_time_ms();
        nova_metal_flash_attention(
            q_data, k_data, v_data, (float*)output_gpu->data,
            batch, num_heads, seq_len, head_dim, scale, true
        );
        double gpu_time = get_time_ms() - gpu_start;
        
        printf("   GPU time: %.2f ms\n", gpu_time);
        printf("   Speedup: %.2fx\n", cpu_time / gpu_time);
        
        extern void nova_metal_flash_attention_cleanup(void);
        nova_metal_flash_attention_cleanup();
    }
#endif
    
    // Cleanup
    nova_tensor_destroy(Q);
    nova_tensor_destroy(K);
    nova_tensor_destroy(V);
    nova_tensor_destroy(output_cpu);
    nova_tensor_destroy(output_gpu);
    
    printf("✅ Performance comparison complete\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════

int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("   GPU FLASH ATTENTION TEST SUITE\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    int failed = 0;
    
    if (test_metal_flash_attention() != 0) failed++;
    if (test_cuda_flash_attention() != 0) failed++;
    if (test_performance_comparison() != 0) failed++;
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    if (failed == 0) {
        printf("   ✅ ALL GPU TESTS PASSED\n");
    } else {
        printf("   ❌ TESTS FAILED: %d\n", failed);
    }
    printf("═══════════════════════════════════════════════════════════════\n");
    
    return failed;
}
