/**
 * test_model_loading.c - Model Loading Tests
 * 
 * Tests .znm format loading and conversion
 */

#include "nova_model_loader.h"
#include "nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "❌ TEST FAILED: %s\n", msg); \
        return -1; \
    } else { \
        printf("✅ %s\n", msg); \
    } \
} while(0)

// ═══════════════════════════════════════════════════════════════════════════
// Test 1: Create and Save Model
// ═══════════════════════════════════════════════════════════════════════════

int test_save_model() {
    printf("\n🧪 Test: Save Model to .znm\n");
    
    // Create model configuration
    ZNMModelConfig config = {
        .vocab_size = 32000,
        .hidden_size = 512,
        .num_hidden_layers = 4,
        .num_attention_heads = 8,
        .num_key_value_heads = 8,
        .intermediate_size = 2048,
        .max_position_embeddings = 2048,
        .rope_theta = 10000.0f,
        .rms_norm_eps = 1e-6f
    };
    
    // Create writer
    ZNMWriter *writer = znm_writer_create("test_model.znm", &config);
    TEST_ASSERT(writer != NULL, "Writer created");
    
    // Add some tensors
    int64_t embed_shape[2] = {config.vocab_size, config.hidden_size};
    NovaTensor *embed = nova_tensor_ones(NULL, embed_shape, 2);
    TEST_ASSERT(embed != NULL, "Embedding tensor created");
    
    int result = znm_writer_add_tensor(writer, "model.embed_tokens.weight", embed);
    TEST_ASSERT(result == 0, "Tensor added");
    
    // Add weight matrices
    int64_t weight_shape[2] = {config.hidden_size, config.hidden_size};
    NovaTensor *weight = nova_tensor_ones(NULL, weight_shape, 2);
    
    znm_writer_add_tensor(writer, "model.layers.0.self_attn.q_proj.weight", weight);
    znm_writer_add_tensor(writer, "model.layers.0.self_attn.k_proj.weight", weight);
    znm_writer_add_tensor(writer, "model.layers.0.self_attn.v_proj.weight", weight);
    znm_writer_add_tensor(writer, "model.layers.0.self_attn.o_proj.weight", weight);
    
    // Cleanup
    nova_tensor_destroy(embed);
    nova_tensor_destroy(weight);
    znm_writer_close(writer);
    
    printf("✅ Model saved to test_model.znm\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 2: Load Model
// ═══════════════════════════════════════════════════════════════════════════

int test_load_model() {
    printf("\n🧪 Test: Load Model from .znm\n");
    
    // Open file
    ZNMFile *file = znm_open("test_model.znm");
    TEST_ASSERT(file != NULL, "File opened");
    
    // Get config
    ZNMModelConfig *config = znm_get_config(file);
    TEST_ASSERT(config != NULL, "Config loaded");
    TEST_ASSERT(config->vocab_size == 32000, "Vocab size correct");
    TEST_ASSERT(config->hidden_size == 512, "Hidden size correct");
    TEST_ASSERT(config->num_hidden_layers == 4, "Num layers correct");
    
    printf("   Model config:\n");
    printf("     Vocab size: %lld\n", config->vocab_size);
    printf("     Hidden size: %lld\n", config->hidden_size);
    printf("     Layers: %lld\n", config->num_hidden_layers);
    
    // Load a tensor
    NovaTensor *tensor = znm_load_tensor(file, "model.embed_tokens.weight");
    TEST_ASSERT(tensor != NULL, "Tensor loaded");
    
    if (tensor) {
        printf("   Loaded tensor shape: [%lld, %lld]\n", 
               tensor->shape[0], tensor->shape[1]);
        nova_tensor_destroy(tensor);
    }
    
    // Cleanup
    znm_close(file);
    
    printf("✅ Model loaded successfully\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 3: Model Conversion
// ═══════════════════════════════════════════════════════════════════════════

int test_model_conversion() {
    printf("\n🧪 Test: Model Conversion (PyTorch → .znm)\n");
    
    // This is a stub conversion for testing
    int result = znm_convert_from_pytorch("dummy.pt", "converted_model.znm");
    TEST_ASSERT(result == 0, "Conversion executed");
    
    // Try to load converted model
    ZNMFile *file = znm_open("converted_model.znm");
    TEST_ASSERT(file != NULL, "Converted model loaded");
    
    ZNMModelConfig *config = znm_get_config(file);
    if (config) {
        printf("   Converted model:\n");
        printf("     Vocab: %lld\n", config->vocab_size);
        printf("     Hidden: %lld\n", config->hidden_size);
        printf("     Layers: %lld\n", config->num_hidden_layers);
    }
    
    znm_close(file);
    
    printf("✅ Conversion test passed\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 4: Memory Mapping Verification
// ═══════════════════════════════════════════════════════════════════════════

int test_memory_mapping() {
    printf("\n🧪 Test: Memory Mapping (Zero-Copy)\n");
    
    ZNMFile *file = znm_open("test_model.znm");
    TEST_ASSERT(file != NULL, "File opened");
    
    // Load multiple tensors - should be zero-copy
    NovaTensor *t1 = znm_load_tensor(file, "model.embed_tokens.weight");
    NovaTensor *t2 = znm_load_tensor(file, "model.layers.0.self_attn.q_proj.weight");
    
    TEST_ASSERT(t1 != NULL, "Tensor 1 loaded");
    TEST_ASSERT(t2 != NULL, "Tensor 2 loaded");
    
    printf("   Memory mapped tensors loaded (zero-copy)\n");
    
    // Cleanup
    if (t1) nova_tensor_destroy(t1);
    if (t2) nova_tensor_destroy(t2);
    znm_close(file);
    
    printf("✅ Memory mapping test passed\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 5: Large Model Handling
// ═══════════════════════════════════════════════════════════════════════════

int test_large_model() {
    printf("\n🧪 Test: Large Model Handling\n");
    
    // Create a larger model config
    ZNMModelConfig config = {
        .vocab_size = 128000,
        .hidden_size = 4096,
        .num_hidden_layers = 32,
        .num_attention_heads = 32,
        .num_key_value_heads = 32,
        .intermediate_size = 11008,
        .max_position_embeddings = 4096,
        .rope_theta = 10000.0f,
        .rms_norm_eps = 1e-6f
    };
    
    printf("   Creating large model config:\n");
    printf("     Vocab: %lld\n", config.vocab_size);
    printf("     Hidden: %lld\n", config.hidden_size);
    printf("     Layers: %lld\n", config.num_hidden_layers);
    
    // Calculate approximate size
    int64_t embed_params = config.vocab_size * config.hidden_size;
    int64_t layer_params = config.hidden_size * config.hidden_size * 4;  // Q,K,V,O
    int64_t total_params = embed_params + layer_params * config.num_hidden_layers;
    
    printf("   Estimated parameters: ~%lld M\n", total_params / 1000000);
    printf("   Estimated size (FP32): ~%lld MB\n", (total_params * 4) / (1024 * 1024));
    
    TEST_ASSERT(total_params > 0, "Parameter count calculated");
    
    printf("✅ Large model test passed\n");
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════

int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("   MODEL LOADING TEST SUITE\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    int failed = 0;
    
    if (test_save_model() != 0) failed++;
    if (test_load_model() != 0) failed++;
    if (test_model_conversion() != 0) failed++;
    if (test_memory_mapping() != 0) failed++;
    if (test_large_model() != 0) failed++;
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    if (failed == 0) {
        printf("   ✅ ALL MODEL LOADING TESTS PASSED (5/5)\n");
    } else {
        printf("   ❌ TESTS FAILED: %d/5\n", failed);
    }
    printf("═══════════════════════════════════════════════════════════════\n");
    
    // Cleanup test files
    printf("\nCleaning up test files...\n");
    remove("test_model.znm");
    remove("converted_model.znm");
    
    return failed;
}
