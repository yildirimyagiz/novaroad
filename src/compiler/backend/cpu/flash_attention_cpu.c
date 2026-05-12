/**
 * flash_attention_cpu.c - CPU-optimized Flash Attention backend
 * 
 * Implements Flash Attention for the CPU backend with SIMD optimizations
 */

// #include "../nova_backend_dispatch.h"


#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#define HAVE_NEON 1
#endif

#ifdef __AVX2__
#include <immintrin.h>
#define HAVE_AVX2 1
#endif

/**
 * CPU Flash Attention kernel with SIMD optimization
 */
int64_t nova_cpu_flash_attention(
    const float *Q,
    const float *K,
    const float *V,
    float *Out,
    int L,      // Sequence length
    int D       // Head dimension
) {
    if (!Q || !K || !V || !Out || L <= 0 || D <= 0) {
        return -1;
    }
    
    // Simple Flash Attention implementation
    // In production, this would use tiled computation and SIMD
    
    float scale = 1.0f / sqrtf((float)D);
    
    // Allocate attention scores
    float *scores = malloc(L * L * sizeof(float));
    if (!scores) return -1;
    
    // Compute Q @ K^T
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < L; j++) {
            float sum = 0.0f;
            
#ifdef HAVE_NEON
            // NEON SIMD optimization for ARM
            int d = 0;
            float32x4_t sum_vec = vdupq_n_f32(0.0f);
            for (; d + 4 <= D; d += 4) {
                float32x4_t q_vec = vld1q_f32(&Q[i * D + d]);
                float32x4_t k_vec = vld1q_f32(&K[j * D + d]);
                sum_vec = vmlaq_f32(sum_vec, q_vec, k_vec);
            }
            sum = vaddvq_f32(sum_vec);
            
            // Remainder
            for (; d < D; d++) {
                sum += Q[i * D + d] * K[j * D + d];
            }
#else
            // Scalar fallback
            for (int d = 0; d < D; d++) {
                sum += Q[i * D + d] * K[j * D + d];
            }
#endif
            
            scores[i * L + j] = sum * scale;
        }
    }
    
    // Apply softmax row-wise
    for (int i = 0; i < L; i++) {
        float *row = &scores[i * L];
        
        // Find max
        float max_val = row[0];
        for (int j = 1; j < L; j++) {
            if (row[j] > max_val) max_val = row[j];
        }
        
        // Exp and sum
        float sum = 0.0f;
        for (int j = 0; j < L; j++) {
            row[j] = expf(row[j] - max_val);
            sum += row[j];
        }
        
        // Normalize
        float inv_sum = 1.0f / sum;
        for (int j = 0; j < L; j++) {
            row[j] *= inv_sum;
        }
    }
    
    // Compute attention @ V
    for (int i = 0; i < L; i++) {
        for (int d = 0; d < D; d++) {
            float sum = 0.0f;
            
#ifdef HAVE_NEON
            int j = 0;
            float32x4_t sum_vec = vdupq_n_f32(0.0f);
            for (; j + 4 <= L; j += 4) {
                float32x4_t attn_vec = vld1q_f32(&scores[i * L + j]);
                float32x4_t v_vec = {
                    V[(j+0) * D + d],
                    V[(j+1) * D + d],
                    V[(j+2) * D + d],
                    V[(j+3) * D + d]
                };
                sum_vec = vmlaq_f32(sum_vec, attn_vec, v_vec);
            }
            sum = vaddvq_f32(sum_vec);
            
            for (; j < L; j++) {
                sum += scores[i * L + j] * V[j * D + d];
            }
#else
            for (int j = 0; j < L; j++) {
                sum += scores[i * L + j] * V[j * D + d];
            }
#endif
            
            Out[i * D + d] = sum;
        }
    }
    
    free(scores);
    return 0;
}
