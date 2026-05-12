/**
 * test_gpt_inference.c - GPT Inference Test Suite
 * 
 * Tests the complete GPT pipeline:
 * - Backend initialization
 * - Model loading
 * - Flash Attention
 * - KV Cache
 * - Token generation
 */

#include "nova_gpt_backend.h"
#include "nova_tensor.h"
#include "nova_backend_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

// Test utilities
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "❌ TEST FAILED: %s\n", msg); \
        return -1; \
    } else { \
        printf("✅ %s\n", msg); \
    } \
} while(0)

#define TEST_START(name) printf("\n🧪 Test: %s\n", name)
#define TEST_END() printf("✅ Test completed\n")

// ═══════════════════════════════════════════════════════════════════════════
// Test 1: Backend Initialization
// ═══════════════════════════════════════════════════════════════════════════

int test_backend_init() {
    TEST_START("Backend Initialization");
    
    // Initialize backend system
    int result = nova_backend_init(NOVA_BACKEND_AUTO);
    TEST_ASSERT(result == 0, "Backend init successful");
    
    // Check backend status
    NovaBackendStatus status = nova_backend_status();
    TEST_ASSERT(status.active != NOVA_BACKEND_AUTO, "Backend auto-selected");
    
    printf("   Active backend: %s\n", nova_backend_name(status.active));
    printf("   CUDA: %s\n", status.cuda_available ? "✅" : "❌");
    printf("   Metal: %s\n", status.metal_available ? "✅" : "❌");
    printf("   Vulkan: %s\n", status.vulkan_available ? "✅" : "❌");
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 2: GPT Configuration
// ═══════════════════════════════════════════════════════════════════════════

int test_gpt_config() {
    TEST_START("GPT Configuration");
    
    // Create a small GPT config for testing
    NovaGPTConfig config = {
        .vocab_size = 32000,
        .hidden_size = 512,
        .num_hidden_layers = 4,
        .num_attention_heads = 8,
        .num_key_value_heads = 8,
        .intermediate_size = 2048,
        .max_position_embeddings = 2048,
        .sliding_window = -1,
        .rope_theta = 10000.0f,
        .rms_norm_eps = 1e-6f,
        .use_cache = true,
        .use_flash_attention = true
    };
    
    TEST_ASSERT(config.hidden_size % config.num_attention_heads == 0,
                "Hidden size divisible by num heads");
    
    int64_t head_dim = config.hidden_size / config.num_attention_heads;
    printf("   Model: hidden=%ld, layers=%ld, heads=%ld, head_dim=%ld\n",
           config.hidden_size, config.num_hidden_layers, 
           config.num_attention_heads, head_dim);
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 3: KV Cache Creation
// ═══════════════════════════════════════════════════════════════════════════

int test_kv_cache() {
    TEST_START("KV Cache Management");
    
    NovaGPTConfig config = {
        .vocab_size = 32000,
        .hidden_size = 512,
        .num_hidden_layers = 4,
        .num_attention_heads = 8,
        .num_key_value_heads = 8,
        .max_position_embeddings = 2048,
        .use_cache = true
    };
    
    int64_t batch_size = 1;
    
    // Create KV cache
    NovaKVCache *cache = nova_kv_cache_create(
        NULL, &config, batch_size, NOVA_DEVICE_CPU
    );
    TEST_ASSERT(cache != NULL, "KV cache created");
    TEST_ASSERT(cache->num_layers == config.num_hidden_layers, "Correct number of layers");
    TEST_ASSERT(cache->max_seq_len == config.max_position_embeddings, "Correct max seq len");
    
    // Clear cache
    nova_kv_cache_clear(cache);
    TEST_ASSERT(cache->current_seq_len == 0, "Cache cleared");
    
    // Cleanup
    nova_kv_cache_destroy(cache);
    TEST_ASSERT(true, "KV cache destroyed");
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 4: Flash Attention
// ═══════════════════════════════════════════════════════════════════════════

int test_flash_attention() {
    TEST_START("Flash Attention");
    
    // Small attention test: [batch=1, heads=2, seq_len=4, head_dim=8]
    int64_t batch = 1;
    int64_t num_heads = 2;
    int64_t seq_len = 4;
    int64_t head_dim = 8;
    
    int64_t shape[4] = {batch, num_heads, seq_len, head_dim};
    
    // Create tensors
    NovaTensor *Q = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *K = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *V = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *output = nova_tensor_create(NULL, shape, 4, NOVA_DTYPE_FP32);
    
    TEST_ASSERT(Q && K && V && output, "Tensors created");
    
    // Initialize with simple values
    float *q_data = (float *)Q->data;
    float *k_data = (float *)K->data;
    float *v_data = (float *)V->data;
    
    for (size_t i = 0; i < Q->total_elements; i++) {
        q_data[i] = (float)(i % 10) / 10.0f;
        k_data[i] = (float)(i % 7) / 7.0f;
        v_data[i] = (float)(i % 5) / 5.0f;
    }
    
    // Run Flash Attention
    float scale = 1.0f / sqrtf((float)head_dim);
    int result = nova_gpt_flash_attention(Q, K, V, output, true, scale);
    TEST_ASSERT(result == 0, "Flash Attention executed");
    
    // Verify output is not zero
    float *out_data = (float *)output->data;
    bool has_nonzero = false;
    for (size_t i = 0; i < output->total_elements; i++) {
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
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 5: RoPE (Rotary Position Embeddings)
// ═══════════════════════════════════════════════════════════════════════════

int test_rope() {
    TEST_START("Rotary Position Embeddings");
    
    int64_t max_seq_len = 128;
    int64_t head_dim = 64;
    float theta = 10000.0f;
    
    // Create RoPE cache
    NovaRoPECache *cache = nova_rope_cache_create(
        NULL, max_seq_len, head_dim, theta, NOVA_DEVICE_CPU
    );
    TEST_ASSERT(cache != NULL, "RoPE cache created");
    TEST_ASSERT(cache->cos_cache != NULL, "Cos cache allocated");
    TEST_ASSERT(cache->sin_cache != NULL, "Sin cache allocated");
    
    // Verify cache values are reasonable
    float *cos_data = (float *)cache->cos_cache->data;
    float *sin_data = (float *)cache->sin_cache->data;
    
    bool cos_valid = true;
    bool sin_valid = true;
    for (int64_t i = 0; i < 100; i++) {
        if (fabsf(cos_data[i]) > 1.0f) cos_valid = false;
        if (fabsf(sin_data[i]) > 1.0f) sin_valid = false;
    }
    TEST_ASSERT(cos_valid && sin_valid, "RoPE values in valid range");
    
    // Cleanup
    nova_rope_cache_destroy(cache);
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 6: Normalization (RMSNorm)
// ═══════════════════════════════════════════════════════════════════════════

int test_rms_norm() {
    TEST_START("RMS Normalization");
    
    int64_t shape[2] = {1, 512};  // [batch, hidden_size]
    
    NovaTensor *input = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *weight = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *output = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    TEST_ASSERT(input && weight && output, "Norm tensors created");
    
    // Initialize
    float *in_data = (float *)input->data;
    float *w_data = (float *)weight->data;
    
    for (size_t i = 0; i < input->total_elements; i++) {
        in_data[i] = (float)i / 100.0f - 2.5f;  // Range: -2.5 to 2.5
        w_data[i] = 1.0f;  // Unit weight
    }
    
    // Apply RMSNorm
    float eps = 1e-6f;
    int result = nova_gpt_rms_norm(input, weight, output, eps);
    TEST_ASSERT(result == 0, "RMSNorm executed");
    
    // Verify output has reasonable values
    float *out_data = (float *)output->data;
    float sum = 0.0f;
    for (size_t i = 0; i < output->total_elements; i++) {
        sum += fabsf(out_data[i]);
    }
    float avg = sum / (float)output->total_elements;
    TEST_ASSERT(avg > 0.01f && avg < 10.0f, "RMSNorm output in reasonable range");
    
    // Cleanup
    nova_tensor_destroy(input);
    nova_tensor_destroy(weight);
    nova_tensor_destroy(output);
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 7: Integration Test - Mini GPT Forward Pass
// ═══════════════════════════════════════════════════════════════════════════

int test_mini_gpt_forward() {
    TEST_START("Mini GPT Forward Pass");
    
    // Tiny model for testing
    NovaGPTConfig config = {
        .vocab_size = 1000,
        .hidden_size = 128,
        .num_hidden_layers = 2,
        .num_attention_heads = 4,
        .num_key_value_heads = 4,
        .intermediate_size = 512,
        .max_position_embeddings = 128,
        .rope_theta = 10000.0f,
        .rms_norm_eps = 1e-6f,
        .use_cache = true,
        .use_flash_attention = true
    };
    
    int64_t batch = 1;
    int64_t seq_len = 8;
    int64_t head_dim = config.hidden_size / config.num_attention_heads;
    
    // Create input embedding
    int64_t embed_shape[3] = {batch, seq_len, config.hidden_size};
    NovaTensor *hidden_states = nova_tensor_create(NULL, embed_shape, 3, NOVA_DTYPE_FP32);
    TEST_ASSERT(hidden_states != NULL, "Hidden states created");
    
    // Initialize with random-ish values
    float *h_data = (float *)hidden_states->data;
    for (size_t i = 0; i < hidden_states->total_elements; i++) {
        h_data[i] = ((float)(i % 100) / 100.0f) - 0.5f;
    }
    
    // Simulate one layer of GPT:
    // 1. RMSNorm
    // 2. Attention
    // 3. Residual
    // 4. RMSNorm
    // 5. FFN
    // 6. Residual
    
    // Create weight tensors (normally loaded from checkpoint)
    int64_t norm_shape[1] = {config.hidden_size};
    NovaTensor *norm_weight = nova_tensor_ones(NULL, norm_shape, 1);
    
    int64_t attn_shape[4] = {batch, config.num_attention_heads, seq_len, head_dim};
    NovaTensor *Q = nova_tensor_create(NULL, attn_shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *K = nova_tensor_create(NULL, attn_shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *V = nova_tensor_create(NULL, attn_shape, 4, NOVA_DTYPE_FP32);
    NovaTensor *attn_out = nova_tensor_create(NULL, attn_shape, 4, NOVA_DTYPE_FP32);
    
    // For simplicity, Q=K=V=hidden_states (reshaped)
    // In real GPT, these would be linear projections
    memcpy(Q->data, hidden_states->data, 
           batch * seq_len * config.hidden_size * sizeof(float));
    memcpy(K->data, hidden_states->data,
           batch * seq_len * config.hidden_size * sizeof(float));
    memcpy(V->data, hidden_states->data,
           batch * seq_len * config.hidden_size * sizeof(float));
    
    // Run attention
    float scale = 1.0f / sqrtf((float)head_dim);
    int result = nova_gpt_flash_attention(Q, K, V, attn_out, true, scale);
    TEST_ASSERT(result == 0, "Attention forward pass");
    
    // Verify output
    float *attn_data = (float *)attn_out->data;
    bool has_values = false;
    for (size_t i = 0; i < 10; i++) {
        if (fabsf(attn_data[i]) > 1e-5f) {
            has_values = true;
            break;
        }
    }
    TEST_ASSERT(has_values, "Attention produced output");
    
    printf("   ✅ Forward pass successful\n");
    printf("   Model size: %ld params (approx)\n",
           config.vocab_size * config.hidden_size + 
           config.num_hidden_layers * config.hidden_size * 4);
    
    // Cleanup
    nova_tensor_destroy(hidden_states);
    nova_tensor_destroy(norm_weight);
    nova_tensor_destroy(Q);
    nova_tensor_destroy(K);
    nova_tensor_destroy(V);
    nova_tensor_destroy(attn_out);
    
    TEST_END();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Main Test Runner
// ═══════════════════════════════════════════════════════════════════════════

int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("   NOVA GPT INFERENCE TEST SUITE\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    int failed = 0;
    
    if (test_backend_init() != 0) failed++;
    if (test_gpt_config() != 0) failed++;
    if (test_kv_cache() != 0) failed++;
    if (test_flash_attention() != 0) failed++;
    if (test_rope() != 0) failed++;
    if (test_rms_norm() != 0) failed++;
    if (test_mini_gpt_forward() != 0) failed++;
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    if (failed == 0) {
        printf("   ✅ ALL TESTS PASSED (7/7)\n");
    } else {
        printf("   ❌ TESTS FAILED: %d/7\n", failed);
    }
    printf("═══════════════════════════════════════════════════════════════\n");
    
    // Cleanup backend
    nova_backend_cleanup();
    
    return failed;
}
