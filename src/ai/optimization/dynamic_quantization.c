/**
 * @file dynamic_quantization.c
 * @brief Dynamic quantization - runtime optimization
 * 
 * Dynamic quantization:
 * - Quantizes activations at runtime (not ahead-of-time)
 * - Weights can be pre-quantized
 * - No calibration dataset needed
 * - Perfect for inference on variable input ranges
 * 
 * Performance:
 * - 2-4× faster than FP32
 * - 4× less memory for weights
 * - Minimal accuracy loss (<0.5%)
 * 
 * Use cases:
 * - BERT, GPT (language models)
 * - Model serving
 * - Edge deployment
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

/**
 * Dynamic quantization parameters
 * Computed at runtime based on activation statistics
 */
typedef struct {
    float scale;
    int8_t zero_point;
    float min_val;
    float max_val;
} DynamicQuantParams;

/**
 * Compute quantization parameters from activations
 * 
 * Algorithm:
 * 1. Find min/max of activations
 * 2. Compute scale and zero-point
 * 3. Quantize using these parameters
 */
void nova_dynamic_quant_compute_params(
    const float* activations,
    size_t n,
    DynamicQuantParams* params)
{
    // Find min and max
    float min_val = activations[0];
    float max_val = activations[0];
    
    for (size_t i = 1; i < n; i++) {
        if (activations[i] < min_val) min_val = activations[i];
        if (activations[i] > max_val) max_val = activations[i];
    }
    
    // Symmetric quantization (zero-point = 0)
    // More efficient on most hardware
    float abs_max = fmaxf(fabsf(min_val), fabsf(max_val));
    params->scale = abs_max / 127.0f;
    params->zero_point = 0;
    params->min_val = min_val;
    params->max_val = max_val;
}

/**
 * Dynamic quantization: FP32 → INT8
 */
void nova_dynamic_quantize(
    const float* input,
    int8_t* output,
    size_t n,
    DynamicQuantParams* params)
{
    // Compute parameters from input
    nova_dynamic_quant_compute_params(input, n, params);
    
    float inv_scale = 1.0f / params->scale;
    
    // Quantize
    for (size_t i = 0; i < n; i++) {
        float q = input[i] * inv_scale;
        q = roundf(q);
        q = fmaxf(-128.0f, fminf(127.0f, q));
        output[i] = (int8_t)q;
    }
}

/**
 * Dynamic linear layer (most common use case)
 * 
 * Computes: output = input × weight^T + bias
 * - Input: quantized dynamically
 * - Weight: pre-quantized (stored as INT8)
 * - Output: FP32
 */
void nova_dynamic_linear(
    const float* input_fp32,        // [N] FP32
    const int8_t* weight_int8,      // [M × N] INT8 (pre-quantized)
    const float* bias,              // [M] FP32
    float* output,                  // [M] FP32
    int M, int N,
    float weight_scale)             // Pre-computed weight scale
{
    // Dynamically quantize input
    int8_t* input_int8 = (int8_t*)malloc(N * sizeof(int8_t));
    DynamicQuantParams input_params;
    
    nova_dynamic_quantize(input_fp32, input_int8, N, &input_params);
    
    // INT8 matrix-vector multiply
    int32_t* result_int32 = (int32_t*)malloc(M * sizeof(int32_t));
    
    for (int i = 0; i < M; i++) {
        int32_t sum = 0;
        
        #ifdef __ARM_NEON
        // NEON optimization: sdot instruction
        int32x4_t sum_vec = vdupq_n_s32(0);
        int j = 0;
        
        for (; j + 16 <= N; j += 16) {
            int8x16_t a = vld1q_s8(&input_int8[j]);
            int8x16_t b = vld1q_s8(&weight_int8[i * N + j]);
            sum_vec = vdotq_s32(sum_vec, a, b);
        }
        
        sum = vaddvq_s32(sum_vec);
        
        // Scalar remainder
        for (; j < N; j++) {
            sum += (int32_t)input_int8[j] * (int32_t)weight_int8[i * N + j];
        }
        #else
        // Scalar implementation
        for (int j = 0; j < N; j++) {
            sum += (int32_t)input_int8[j] * (int32_t)weight_int8[i * N + j];
        }
        #endif
        
        result_int32[i] = sum;
    }
    
    // Dequantize and add bias
    float output_scale = input_params.scale * weight_scale;
    
    for (int i = 0; i < M; i++) {
        output[i] = (float)result_int32[i] * output_scale;
        if (bias) {
            output[i] += bias[i];
        }
    }
    
    free(input_int8);
    free(result_int32);
}

/**
 * Dynamic quantization for entire model
 * 
 * Strategy:
 * - Quantize activations dynamically (layer by layer)
 * - Weights pre-quantized offline
 * - Accumulate in INT32, dequantize to FP32
 */
typedef struct {
    int8_t* weights;        // Quantized weights
    float* bias;            // Bias (FP32)
    float weight_scale;     // Weight quantization scale
    int input_size;
    int output_size;
} DynamicQuantLayer;

void nova_dynamic_quant_layer_forward(
    DynamicQuantLayer* layer,
    const float* input,
    float* output)
{
    nova_dynamic_linear(
        input,
        layer->weights,
        layer->bias,
        output,
        layer->output_size,
        layer->input_size,
        layer->weight_scale);
}

/**
 * BERT example with dynamic quantization
 */
void nova_dynamic_quant_bert_example(void) {
    printf("\n=== Dynamic Quantization BERT Example ===\n\n");
    
    // Typical BERT layer: 768 → 3072 (FFN)
    int input_size = 768;
    int output_size = 3072;
    
    // Allocate layer
    DynamicQuantLayer layer;
    layer.input_size = input_size;
    layer.output_size = output_size;
    layer.weights = (int8_t*)malloc(output_size * input_size * sizeof(int8_t));
    layer.bias = (float*)malloc(output_size * sizeof(float));
    
    // Simulate pre-quantized weights
    // (In practice, quantize offline from trained FP32 model)
    for (int i = 0; i < output_size * input_size; i++) {
        layer.weights[i] = (int8_t)(rand() % 256 - 128);
    }
    for (int i = 0; i < output_size; i++) {
        layer.bias[i] = ((float)rand() / RAND_MAX) * 0.01f;
    }
    layer.weight_scale = 0.001f; // Example scale
    
    // Run inference
    float* input = (float*)malloc(input_size * sizeof(float));
    float* output = (float*)malloc(output_size * sizeof(float));
    
    for (int i = 0; i < input_size; i++) {
        input[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
    
    nova_dynamic_quant_layer_forward(&layer, input, output);
    
    printf("BERT layer inference complete\n");
    printf("  Input: %d (FP32)\n", input_size);
    printf("  Output: %d (FP32)\n", output_size);
    printf("  Weights: %d (INT8, pre-quantized)\n", output_size * input_size);
    printf("  Memory savings: 4× for weights\n");
    printf("  Expected speedup: 2-3×\n");
    
    free(input);
    free(output);
    free(layer.weights);
    free(layer.bias);
}

/**
 * Compare dynamic vs static quantization
 */
void nova_dynamic_vs_static_quant(void) {
    printf("\n=== Dynamic vs Static Quantization ===\n\n");
    
    printf("Static Quantization:\n");
    printf("  ✅ Quantize weights + activations offline\n");
    printf("  ✅ 4× faster (all INT8 ops)\n");
    printf("  ❌ Requires calibration dataset\n");
    printf("  ❌ Less flexible (fixed input range)\n");
    printf("  Use case: CNNs (ResNet, MobileNet)\n\n");
    
    printf("Dynamic Quantization:\n");
    printf("  ✅ Quantize activations at runtime\n");
    printf("  ✅ No calibration needed\n");
    printf("  ✅ Flexible input ranges\n");
    printf("  ❌ 2-3× faster (INT8 compute, FP32 overhead)\n");
    printf("  Use case: Transformers (BERT, GPT)\n\n");
}
