/**
 * @file mixed_precision.c
 * @brief Mixed precision training and inference
 * 
 * Mixed precision:
 * - FP16 for forward/backward (2× faster, 2× less memory)
 * - FP32 for master weights and optimizer state
 * - Loss scaling to prevent underflow
 * 
 * Benefits:
 * - 2-3× faster training
 * - 2× less GPU memory (can train larger models!)
 * - Same accuracy as FP32
 * 
 * Hardware support:
 * - NVIDIA: Tensor Cores (V100, A100, H100)
 * - AMD: Matrix Cores (MI100+)
 * - Apple: M1/M2/M3 (native FP16 support)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <float.h>

#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
#include <arm_neon.h>
#include <arm_fp16.h>
typedef __fp16 float16_t;
#else
typedef uint16_t float16_t;
#endif

/**
 * Mixed precision training configuration
 */
typedef struct {
    float loss_scale;           // Loss scaling factor (typically 1024-65536)
    float loss_scale_window;    // Dynamic loss scaling window
    int fp16_gradients;         // Store gradients in FP16
    int fp32_master_weights;    // Keep FP32 master copy
} MixedPrecisionConfig;

/**
 * FP32 to FP16 conversion (software)
 */
static inline float16_t fp32_to_fp16_sw(float x) {
    #ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return (__fp16)x;
    #else
    // Software FP16 conversion (IEEE 754)
    union { float f; uint32_t i; } u = {x};
    uint32_t sign = (u.i >> 16) & 0x8000;
    uint32_t exp = ((u.i >> 23) & 0xff) - 127 + 15;
    uint32_t mantissa = (u.i >> 13) & 0x3ff;
    
    if (exp <= 0) return (float16_t)sign; // Underflow
    if (exp >= 31) return (float16_t)(sign | 0x7c00); // Overflow (inf)
    
    return (float16_t)(sign | (exp << 10) | mantissa);
    #endif
}

/**
 * FP16 to FP32 conversion (software)
 */
static inline float fp16_to_fp32_sw(float16_t x) {
    #ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
    return (float)x;
    #else
    uint16_t h = x;
    uint32_t sign = (h & 0x8000) << 16;
    uint32_t exp = ((h >> 10) & 0x1f);
    uint32_t mantissa = (h & 0x3ff);
    
    if (exp == 0) { // Subnormal or zero
        if (mantissa == 0) {
            union { float f; uint32_t i; } u = {0};
            u.i = sign;
            return u.f;
        }
        // Subnormal: not fully implemented
    }
    
    exp = exp - 15 + 127;
    mantissa = mantissa << 13;
    
    union { float f; uint32_t i; } u;
    u.i = sign | (exp << 23) | mantissa;
    return u.f;
    #endif
}

/**
 * Mixed precision forward pass
 * 
 * Input: FP32
 * Compute: FP16 (faster!)
 * Output: FP32 (for loss computation)
 */
void nova_mixed_precision_linear_forward(
    const float* input_fp32,    // [N]
    const float16_t* weight_fp16, // [M × N]
    const float16_t* bias_fp16,   // [M]
    float* output_fp32,         // [M]
    int M, int N)
{
    // Convert input to FP16
    float16_t* input_fp16 = (float16_t*)malloc(N * sizeof(float16_t));
    
    #ifdef __ARM_NEON
    // NEON vectorized conversion
    for (int i = 0; i < N; i += 4) {
        float32x4_t v32 = vld1q_f32(&input_fp32[i]);
        float16x4_t v16 = vcvt_f16_f32(v32);
        vst1_f16(&input_fp16[i], v16);
    }
    #else
    for (int i = 0; i < N; i++) {
        input_fp16[i] = fp32_to_fp16_sw(input_fp32[i]);
    }
    #endif
    
    // FP16 matrix-vector multiply
    float16_t* output_fp16 = (float16_t*)malloc(M * sizeof(float16_t));
    
    for (int i = 0; i < M; i++) {
        #ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
        float16x8_t sum = vdupq_n_f16(0);
        int j = 0;
        
        for (; j + 8 <= N; j += 8) {
            float16x8_t w = vld1q_f16(&weight_fp16[i * N + j]);
            float16x8_t x = vld1q_f16(&input_fp16[j]);
            sum = vfmaq_f16(sum, w, x);
        }
        
        float16_t result = vaddvq_f16(sum);
        
        // Scalar remainder
        for (; j < N; j++) {
            result += weight_fp16[i * N + j] * input_fp16[j];
        }
        
        output_fp16[i] = result + bias_fp16[i];
        #else
        // Software FP16 compute
        float sum = 0.0f;
        for (int j = 0; j < N; j++) {
            sum += fp16_to_fp32_sw(weight_fp16[i * N + j]) *
                   fp16_to_fp32_sw(input_fp16[j]);
        }
        output_fp16[i] = fp32_to_fp16_sw(sum + fp16_to_fp32_sw(bias_fp16[i]));
        #endif
    }
    
    // Convert output back to FP32
    for (int i = 0; i < M; i++) {
        output_fp32[i] = fp16_to_fp32_sw(output_fp16[i]);
    }
    
    free(input_fp16);
    free(output_fp16);
}

/**
 * Loss scaling for mixed precision training
 * 
 * Scale loss up before backward pass to prevent gradient underflow
 * Scale gradients down after backward pass
 */
void nova_mixed_precision_scale_loss(
    float* loss,
    float scale)
{
    *loss *= scale;
}

void nova_mixed_precision_unscale_gradients(
    float* gradients,
    size_t n,
    float scale)
{
    float inv_scale = 1.0f / scale;
    for (size_t i = 0; i < n; i++) {
        gradients[i] *= inv_scale;
    }
}

/**
 * Dynamic loss scaling
 * 
 * Automatically adjust loss scale based on gradient overflow detection
 */
void nova_mixed_precision_dynamic_loss_scale(
    MixedPrecisionConfig* config,
    const float* gradients,
    size_t n,
    int* overflow_detected)
{
    // Check for overflow (inf or nan)
    *overflow_detected = 0;
    for (size_t i = 0; i < n; i++) {
        if (isinf(gradients[i]) || isnan(gradients[i])) {
            *overflow_detected = 1;
            break;
        }
    }
    
    if (*overflow_detected) {
        // Reduce loss scale
        config->loss_scale *= 0.5f;
        printf("Gradient overflow detected! Reducing loss scale to %.0f\n",
               config->loss_scale);
    } else {
        // Gradually increase loss scale
        config->loss_scale_window += 1.0f;
        if (config->loss_scale_window >= 2000.0f) {
            config->loss_scale *= 2.0f;
            config->loss_scale_window = 0.0f;
            printf("Increasing loss scale to %.0f\n", config->loss_scale);
        }
    }
    
    // Clamp loss scale
    if (config->loss_scale < 1.0f) config->loss_scale = 1.0f;
    if (config->loss_scale > 65536.0f) config->loss_scale = 65536.0f;
}

/**
 * Mixed precision optimizer step (e.g., SGD)
 * 
 * Maintains FP32 master weights for numerical stability
 */
void nova_mixed_precision_optimizer_step(
    float* master_weights_fp32,     // [N] FP32 master copy
    float16_t* model_weights_fp16,  // [N] FP16 working copy
    const float* gradients_fp32,    // [N] FP32 gradients
    size_t n,
    float learning_rate)
{
    // Update FP32 master weights
    for (size_t i = 0; i < n; i++) {
        master_weights_fp32[i] -= learning_rate * gradients_fp32[i];
    }
    
    // Copy to FP16 model weights
    for (size_t i = 0; i < n; i++) {
        model_weights_fp16[i] = fp32_to_fp16_sw(master_weights_fp32[i]);
    }
}

/**
 * Example: Mixed precision training loop
 */
void nova_mixed_precision_training_example(void) {
    printf("\n=== Mixed Precision Training Example ===\n\n");
    
    MixedPrecisionConfig config = {
        .loss_scale = 1024.0f,
        .loss_scale_window = 0.0f,
        .fp16_gradients = 1,
        .fp32_master_weights = 1
    };
    
    int N = 1000;  // Number of weights
    
    // Allocate
    float* master_weights = (float*)malloc(N * sizeof(float));
    float16_t* model_weights = (float16_t*)malloc(N * sizeof(float16_t));
    float* gradients = (float*)malloc(N * sizeof(float));
    
    // Initialize
    for (int i = 0; i < N; i++) {
        master_weights[i] = ((float)rand() / RAND_MAX) * 0.01f;
        model_weights[i] = fp32_to_fp16_sw(master_weights[i]);
        gradients[i] = ((float)rand() / RAND_MAX) * 0.001f;
    }
    
    // Simulate training step
    float loss = 2.5f;
    
    // 1. Scale loss
    nova_mixed_precision_scale_loss(&loss, config.loss_scale);
    printf("Step 1: Scaled loss = %.2f (scale = %.0f)\n", loss, config.loss_scale);
    
    // 2. Backward pass (simulated, gradients already computed)
    printf("Step 2: Backward pass in FP16\n");
    
    // 3. Unscale gradients
    nova_mixed_precision_unscale_gradients(gradients, N, config.loss_scale);
    printf("Step 3: Unscaled gradients\n");
    
    // 4. Check for overflow
    int overflow;
    nova_mixed_precision_dynamic_loss_scale(&config, gradients, N, &overflow);
    
    if (!overflow) {
        // 5. Optimizer step
        nova_mixed_precision_optimizer_step(master_weights, model_weights,
                                           gradients, N, 0.01f);
        printf("Step 4: Optimizer step (FP32 master, FP16 model)\n");
    }
    
    printf("\nMemory usage:\n");
    printf("  FP32 only:      %zu KB\n", (N * sizeof(float) * 2) / 1024);
    printf("  Mixed precision: %zu KB\n",
           (N * sizeof(float) + N * sizeof(float16_t)) / 1024);
    printf("  Savings:        %.1f×\n",
           (float)(N * sizeof(float) * 2) / (N * sizeof(float) + N * sizeof(float16_t)));
    
    free(master_weights);
    free(model_weights);
    free(gradients);
}
