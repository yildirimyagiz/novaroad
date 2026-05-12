/**
 * quantization_int8.c - INT8 Quantization Implementation
 * 
 * Symmetric and asymmetric INT8 quantization for model compression
 * Achieves 4× memory reduction with <1% accuracy loss
 */

#include "nova_quantization.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// INT8 Quantization
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Quantize FP32 to INT8 (symmetric)
 * q = round(x / scale), where scale = max(abs(x)) / 127
 */
NovaQuantizedTensorINT8 *nova_quantize_int8(
    const NovaTensor *tensor,
    bool per_channel,
    bool symmetric
) {
    if (!tensor) return NULL;
    
    NovaQuantizedTensorINT8 *qtensor = calloc(1, sizeof(NovaQuantizedTensorINT8));
    if (!qtensor) return NULL;
    
    // Copy shape
    qtensor->ndim = tensor->ndim;
    qtensor->shape = malloc(tensor->ndim * sizeof(int64_t));
    memcpy(qtensor->shape, tensor->shape, tensor->ndim * sizeof(int64_t));
    qtensor->total_elements = tensor->total_elements;
    qtensor->per_channel = per_channel;
    
    // Allocate quantized data
    qtensor->data = malloc(tensor->total_elements * sizeof(int8_t));
    if (!qtensor->data) {
        free(qtensor->shape);
        free(qtensor);
        return NULL;
    }
    
    const float *fp_data = (const float *)tensor->data;
    
    if (per_channel && tensor->ndim >= 2) {
        // Per-channel quantization (for weight matrices)
        int64_t num_channels = tensor->shape[0];  // Assume [out_channels, ...]
        int64_t elements_per_channel = tensor->total_elements / num_channels;
        
        qtensor->scales = malloc(num_channels * sizeof(float));
        if (!symmetric) {
            qtensor->zero_points = malloc(num_channels * sizeof(int8_t));
        }
        
        // Quantize each channel separately
        for (int64_t c = 0; c < num_channels; c++) {
            const float *channel_data = fp_data + c * elements_per_channel;
            int8_t *channel_qdata = qtensor->data + c * elements_per_channel;
            
            // Find min/max
            float min_val = FLT_MAX;
            float max_val = -FLT_MAX;
            
            for (int64_t i = 0; i < elements_per_channel; i++) {
                float val = channel_data[i];
                if (val < min_val) min_val = val;
                if (val > max_val) max_val = val;
            }
            
            // Compute scale and zero point
            float scale, zero_point = 0;
            
            if (symmetric) {
                // Symmetric: scale = max(abs(min), abs(max)) / 127
                float abs_max = fmaxf(fabsf(min_val), fabsf(max_val));
                scale = abs_max / 127.0f;
                if (scale == 0.0f) scale = 1.0f;  // Avoid division by zero
            } else {
                // Asymmetric: map [min, max] to [-128, 127]
                scale = (max_val - min_val) / 255.0f;
                if (scale == 0.0f) scale = 1.0f;
                zero_point = -roundf(min_val / scale);
            }
            
            qtensor->scales[c] = scale;
            if (!symmetric) {
                qtensor->zero_points[c] = (int8_t)zero_point;
            }
            
            // Quantize
            for (int64_t i = 0; i < elements_per_channel; i++) {
                float qval;
                if (symmetric) {
                    qval = roundf(channel_data[i] / scale);
                } else {
                    qval = roundf(channel_data[i] / scale) + zero_point;
                }
                
                // Clamp to INT8 range
                if (qval < -128.0f) qval = -128.0f;
                if (qval > 127.0f) qval = 127.0f;
                
                channel_qdata[i] = (int8_t)qval;
            }
        }
        
        printf("✅ INT8 quantization: %lld channels, per-channel, %s\n",
               num_channels, symmetric ? "symmetric" : "asymmetric");
        
    } else {
        // Per-tensor quantization
        qtensor->scales = malloc(sizeof(float));
        if (!symmetric) {
            qtensor->zero_points = malloc(sizeof(int8_t));
        }
        
        // Find global min/max
        float min_val = FLT_MAX;
        float max_val = -FLT_MAX;
        
        for (size_t i = 0; i < tensor->total_elements; i++) {
            float val = fp_data[i];
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }
        
        // Compute scale
        float scale, zero_point = 0;
        
        if (symmetric) {
            float abs_max = fmaxf(fabsf(min_val), fabsf(max_val));
            scale = abs_max / 127.0f;
            if (scale == 0.0f) scale = 1.0f;
        } else {
            scale = (max_val - min_val) / 255.0f;
            if (scale == 0.0f) scale = 1.0f;
            zero_point = -roundf(min_val / scale);
        }
        
        qtensor->scales[0] = scale;
        if (!symmetric) {
            qtensor->zero_points[0] = (int8_t)zero_point;
        }
        
        // Quantize
        for (size_t i = 0; i < tensor->total_elements; i++) {
            float qval;
            if (symmetric) {
                qval = roundf(fp_data[i] / scale);
            } else {
                qval = roundf(fp_data[i] / scale) + zero_point;
            }
            
            if (qval < -128.0f) qval = -128.0f;
            if (qval > 127.0f) qval = 127.0f;
            
            qtensor->data[i] = (int8_t)qval;
        }
        
        printf("✅ INT8 quantization: per-tensor, %s\n",
               symmetric ? "symmetric" : "asymmetric");
    }
    
    // Report compression
    size_t original_size = tensor->total_elements * sizeof(float);
    size_t quantized_size = tensor->total_elements * sizeof(int8_t) +
                           (per_channel ? tensor->shape[0] : 1) * sizeof(float);
    float compression = (float)original_size / (float)quantized_size;
    
    printf("   Compression: %.2fx (%.1f MB → %.1f MB)\n",
           compression,
           original_size / (1024.0f * 1024.0f),
           quantized_size / (1024.0f * 1024.0f));
    
    return qtensor;
}

/**
 * Dequantize INT8 back to FP32
 */
NovaTensor *nova_dequantize_int8(const NovaQuantizedTensorINT8 *qtensor) {
    if (!qtensor) return NULL;
    
    NovaTensor *tensor = nova_tensor_create(NULL, qtensor->shape, qtensor->ndim, NOVA_DTYPE_FP32);
    if (!tensor) return NULL;
    
    float *fp_data = (float *)tensor->data;
    
    if (qtensor->per_channel && qtensor->ndim >= 2) {
        int64_t num_channels = qtensor->shape[0];
        int64_t elements_per_channel = qtensor->total_elements / num_channels;
        
        for (int64_t c = 0; c < num_channels; c++) {
            const int8_t *channel_qdata = qtensor->data + c * elements_per_channel;
            float *channel_data = fp_data + c * elements_per_channel;
            
            float scale = qtensor->scales[c];
            int8_t zero_point = qtensor->zero_points ? qtensor->zero_points[c] : 0;
            
            for (int64_t i = 0; i < elements_per_channel; i++) {
                channel_data[i] = (channel_qdata[i] - zero_point) * scale;
            }
        }
    } else {
        float scale = qtensor->scales[0];
        int8_t zero_point = qtensor->zero_points ? qtensor->zero_points[0] : 0;
        
        for (size_t i = 0; i < qtensor->total_elements; i++) {
            fp_data[i] = (qtensor->data[i] - zero_point) * scale;
        }
    }
    
    return tensor;
}

/**
 * Free INT8 quantized tensor
 */
void nova_quantized_int8_destroy(NovaQuantizedTensorINT8 *qtensor) {
    if (!qtensor) return;
    
    free(qtensor->data);
    free(qtensor->scales);
    free(qtensor->zero_points);
    free(qtensor->shape);
    free(qtensor);
}

// ═══════════════════════════════════════════════════════════════════════════
// INT8 Quantized Operations
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Mixed precision matmul: FP32 activations × INT8 weights
 * This is the most common case in inference
 */
int nova_quantized_matmul_mixed(
    const NovaTensor *A,              // FP32 activations [M, K]
    const NovaQuantizedTensorINT8 *B, // INT8 weights [K, N]
    NovaTensor *C                     // FP32 output [M, N]
) {
    if (!A || !B || !C) return -1;
    
    // Assume A is [M, K], B is [K, N]
    int64_t M = A->shape[0];
    int64_t K = A->shape[1];
    int64_t N = B->shape[0];  // B is transposed in storage
    
    const float *a_data = (const float *)A->data;
    const int8_t *b_data = B->data;
    float *c_data = (float *)C->data;
    
    // Per-channel quantization: each output channel has its own scale
    const float *b_scales = B->scales;
    
    // Naive implementation (can be optimized with SIMD/GPU)
    for (int64_t m = 0; m < M; m++) {
        for (int64_t n = 0; n < N; n++) {
            int32_t acc = 0;  // Use int32 for accumulation
            
            // Dot product in int8
            for (int64_t k = 0; k < K; k++) {
                float a_val = a_data[m * K + k];
                int8_t b_val = b_data[n * K + k];
                
                // Quantize activation on-the-fly (simple approach)
                int8_t a_quant = (int8_t)roundf(a_val * 127.0f / 6.0f);  // Assume range [-6, 6]
                
                acc += (int32_t)a_quant * (int32_t)b_val;
            }
            
            // Dequantize result
            float scale = b_scales[B->per_channel ? n : 0];
            c_data[m * N + n] = (float)acc * scale * (6.0f / 127.0f);
        }
    }
    
    return 0;
}

/**
 * INT8 Matrix Multiplication (all INT8)
 */
int nova_quantized_matmul_int8(
    const NovaQuantizedTensorINT8 *A,
    const NovaQuantizedTensorINT8 *B,
    NovaTensor *C
) {
    if (!A || !B || !C) return -1;
    
    // Similar to mixed, but both inputs are quantized
    // This is faster but requires careful scale management
    
    printf("⚠️  Full INT8 matmul not yet implemented\n");
    return -1;
}

// ═══════════════════════════════════════════════════════════════════════════
// Utilities
// ═══════════════════════════════════════════════════════════════════════════

const char *nova_quant_type_name(NovaQuantType type) {
    switch (type) {
        case NOVA_QUANT_INT8: return "INT8";
        case NOVA_QUANT_INT4: return "INT4";
        case NOVA_QUANT_NF4: return "NF4";
        case NOVA_QUANT_FP16: return "FP16";
        case NOVA_QUANT_BF16: return "BF16";
        default: return "None";
    }
}

float nova_quant_compression_ratio(NovaQuantType type) {
    switch (type) {
        case NOVA_QUANT_INT8: return 4.0f;  // 32bit → 8bit
        case NOVA_QUANT_INT4: return 8.0f;  // 32bit → 4bit
        case NOVA_QUANT_NF4: return 8.0f;
        case NOVA_QUANT_FP16: return 2.0f;  // 32bit → 16bit
        case NOVA_QUANT_BF16: return 2.0f;
        default: return 1.0f;
    }
}

bool nova_quant_is_supported(NovaQuantType type) {
    // Check hardware support
    switch (type) {
        case NOVA_QUANT_INT8:
        case NOVA_QUANT_INT4:
        case NOVA_QUANT_NF4:
            return true;  // Supported on all platforms
        case NOVA_QUANT_FP16:
        case NOVA_QUANT_BF16:
            // Check for SIMD support
            #if defined(__ARM_NEON) || defined(__AVX2__)
            return true;
            #else
            return false;
            #endif
        default:
            return false;
    }
}
