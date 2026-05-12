/**
 * lora.c - LoRA (Low-Rank Adaptation) Implementation
 * 
 * Efficient fine-tuning by training small adapter matrices
 * Reduces trainable parameters by 10,000× with minimal accuracy loss
 */

#include "ml/nova_tensor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════
// LoRA Layer Structure
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    NovaTensor *base_weight;   // Frozen base model weight [out, in]
    NovaTensor *lora_A;        // LoRA down-projection [rank, in]
    NovaTensor *lora_B;        // LoRA up-projection [out, rank]
    int rank;                  // LoRA rank (typically 8-64)
    float alpha;               // Scaling factor
    float scale;               // alpha / rank
    bool enabled;
} NovaLoRALayer;

/**
 * Create LoRA layer
 * 
 * For weight W [out_dim, in_dim]:
 * - Freeze W
 * - Add trainable A [rank, in_dim] and B [out_dim, rank]
 * - Forward: W @ x + (B @ A) @ x * scale
 */
NovaLoRALayer *nova_lora_create(
    NovaTensor *base_weight,
    int rank,
    float alpha
) {
    if (!base_weight || rank <= 0) return NULL;
    
    NovaLoRALayer *layer = calloc(1, sizeof(NovaLoRALayer));
    if (!layer) return NULL;
    
    layer->base_weight = base_weight;
    layer->rank = rank;
    layer->alpha = alpha;
    layer->scale = alpha / (float)rank;
    layer->enabled = true;
    
    // Create LoRA matrices
    int64_t out_dim = base_weight->shape[0];
    int64_t in_dim = base_weight->shape[1];
    
    int64_t shape_A[2] = {rank, in_dim};
    int64_t shape_B[2] = {out_dim, rank};
    
    layer->lora_A = nova_tensor_create(NULL, shape_A, 2, NOVA_DTYPE_FP32);
    layer->lora_B = nova_tensor_create(NULL, shape_B, 2, NOVA_DTYPE_FP32);
    
    if (!layer->lora_A || !layer->lora_B) {
        nova_tensor_destroy(layer->lora_A);
        nova_tensor_destroy(layer->lora_B);
        free(layer);
        return NULL;
    }
    
    // Initialize A with random values, B with zeros
    float *a_data = (float *)layer->lora_A->data;
    float *b_data = (float *)layer->lora_B->data;
    
    // Kaiming initialization for A
    float std = sqrtf(2.0f / (float)in_dim);
    for (size_t i = 0; i < layer->lora_A->total_elements; i++) {
        a_data[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * std;
    }
    
    // Zero initialization for B
    memset(b_data, 0, layer->lora_B->total_elements * sizeof(float));
    
    printf("✅ LoRA layer created: rank=%d, alpha=%.1f, scale=%.4f\n",
           rank, alpha, layer->scale);
    printf("   Base: [%lld, %lld], Trainable: %lld params (%.2f%% of base)\n",
           out_dim, in_dim,
           (int64_t)rank * (out_dim + in_dim),
           100.0f * rank * (out_dim + in_dim) / (float)(out_dim * in_dim));
    
    return layer;
}

/**
 * Forward pass with LoRA
 * 
 * y = W @ x + B @ (A @ x) * scale
 */
NovaTensor *nova_lora_forward(
    const NovaLoRALayer *layer,
    const NovaTensor *input
) {
    if (!layer || !input) return NULL;
    
    // Base forward: W @ x
    NovaTensor *base_out = nova_tensor_matmul(layer->base_weight, input);
    
    if (!layer->enabled || !layer->lora_A || !layer->lora_B) {
        return base_out;
    }
    
    // LoRA path: A @ x
    NovaTensor *lora_hidden = nova_tensor_matmul(layer->lora_A, input);
    
    // B @ lora_hidden
    NovaTensor *lora_out = nova_tensor_matmul(layer->lora_B, lora_hidden);
    
    // Scale and add
    float *lora_data = (float *)lora_out->data;
    float *base_data = (float *)base_out->data;
    
    for (size_t i = 0; i < lora_out->total_elements; i++) {
        base_data[i] += lora_data[i] * layer->scale;
    }
    
    // Cleanup
    nova_tensor_destroy(lora_hidden);
    nova_tensor_destroy(lora_out);
    
    return base_out;
}

/**
 * Get trainable parameters (A and B only)
 */
int nova_lora_get_trainable_params(
    const NovaLoRALayer *layer,
    NovaTensor ***params,
    int *num_params
) {
    if (!layer || !params || !num_params) return -1;
    
    *num_params = 2;
    *params = malloc(2 * sizeof(NovaTensor*));
    (*params)[0] = layer->lora_A;
    (*params)[1] = layer->lora_B;
    
    return 0;
}

/**
 * Merge LoRA weights into base weight
 * W' = W + B @ A * scale
 */
int nova_lora_merge(NovaLoRALayer *layer) {
    if (!layer || !layer->lora_A || !layer->lora_B) return -1;
    
    // Compute B @ A
    NovaTensor *merged_delta = nova_tensor_matmul(layer->lora_B, layer->lora_A);
    if (!merged_delta) return -1;
    
    // Add to base weight with scaling
    float *base_data = (float *)layer->base_weight->data;
    float *delta_data = (float *)merged_delta->data;
    
    for (size_t i = 0; i < layer->base_weight->total_elements; i++) {
        base_data[i] += delta_data[i] * layer->scale;
    }
    
    nova_tensor_destroy(merged_delta);
    
    printf("✅ LoRA weights merged into base model\n");
    
    return 0;
}

/**
 * Free LoRA layer
 */
void nova_lora_destroy(NovaLoRALayer *layer) {
    if (!layer) return;
    
    nova_tensor_destroy(layer->lora_A);
    nova_tensor_destroy(layer->lora_B);
    free(layer);
}

// ═══════════════════════════════════════════════════════════════════════════
// QLoRA (Quantized LoRA)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create QLoRA layer
 * 
 * Base model is quantized to NF4, LoRA adapters are FP32
 * Achieves same quality as LoRA with 4× less memory
 */
typedef struct {
    void *quantized_base;      // NF4 quantized base weight
    NovaTensor *lora_A;
    NovaTensor *lora_B;
    int rank;
    float alpha;
    float scale;
} NovaQLoRALayer;

NovaQLoRALayer *nova_qlora_create(
    void *quantized_base,
    int out_dim,
    int in_dim,
    int rank,
    float alpha
) {
    NovaQLoRALayer *layer = calloc(1, sizeof(NovaQLoRALayer));
    if (!layer) return NULL;
    
    layer->quantized_base = quantized_base;
    layer->rank = rank;
    layer->alpha = alpha;
    layer->scale = alpha / (float)rank;
    
    // Create LoRA adapters (FP32, not quantized)
    int64_t shape_A[2] = {rank, in_dim};
    int64_t shape_B[2] = {out_dim, rank};
    
    layer->lora_A = nova_tensor_create(NULL, shape_A, 2, NOVA_DTYPE_FP32);
    layer->lora_B = nova_tensor_create(NULL, shape_B, 2, NOVA_DTYPE_FP32);
    
    // Initialize (same as LoRA)
    float *a_data = (float *)layer->lora_A->data;
    float std = sqrtf(2.0f / (float)in_dim);
    for (size_t i = 0; i < layer->lora_A->total_elements; i++) {
        a_data[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * std;
    }
    
    memset(layer->lora_B->data, 0, layer->lora_B->total_elements * sizeof(float));
    
    int64_t base_params = (int64_t)out_dim * in_dim;
    int64_t lora_params = rank * (out_dim + in_dim);
    
    printf("✅ QLoRA layer created: rank=%d\n", rank);
    printf("   Base: NF4 quantized (%lld params, 0.5 bytes each)\n", base_params);
    printf("   Trainable: %lld params FP32 (%.3f%% of base)\n",
           lora_params, 100.0f * lora_params / (float)base_params);
    printf("   Memory: %.1f MB (vs %.1f MB for full FP32)\n",
           (base_params * 0.5f + lora_params * 4) / (1024.0f * 1024.0f),
           base_params * 4 / (1024.0f * 1024.0f));
    
    return layer;
}

void nova_qlora_destroy(NovaQLoRALayer *layer) {
    if (!layer) return;
    nova_tensor_destroy(layer->lora_A);
    nova_tensor_destroy(layer->lora_B);
    free(layer);
}
