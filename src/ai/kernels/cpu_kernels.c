/**
 * @file cpu_kernels.c
 * @brief Optimized CPU kernels (fallback when no CUDA)
 */

#include "ai/tensor.h"
#include <math.h>
#include <string.h>

/* Optimized CPU matmul with cache blocking */
void nova_cpu_matmul(const float *A, const float *B, float *C,
                    int M, int N, int K) {
    const int BLOCK = 64;
    
    memset(C, 0, M * N * sizeof(float));
    
    for (int i0 = 0; i0 < M; i0 += BLOCK) {
        for (int j0 = 0; j0 < N; j0 += BLOCK) {
            for (int k0 = 0; k0 < K; k0 += BLOCK) {
                for (int i = i0; i < fmin(i0 + BLOCK, M); i++) {
                    for (int j = j0; j < fmin(j0 + BLOCK, N); j++) {
                        float sum = C[i * N + j];
                        for (int k = k0; k < fmin(k0 + BLOCK, K); k++) {
                            sum += A[i * K + k] * B[k * N + j];
                        }
                        C[i * N + j] = sum;
                    }
                }
            }
        }
    }
}

/* CPU LayerNorm */
void nova_cpu_layernorm(const float *x, float *out,
                       const float *gamma, const float *beta,
                       int batch_size, int hidden_dim, float eps) {
    for (int b = 0; b < batch_size; b++) {
        const float *x_batch = x + b * hidden_dim;
        float *out_batch = out + b * hidden_dim;
        
        float mean = 0.0f;
        for (int i = 0; i < hidden_dim; i++) {
            mean += x_batch[i];
        }
        mean /= hidden_dim;
        
        float var = 0.0f;
        for (int i = 0; i < hidden_dim; i++) {
            float diff = x_batch[i] - mean;
            var += diff * diff;
        }
        var /= hidden_dim;
        
        float inv_std = 1.0f / sqrtf(var + eps);
        for (int i = 0; i < hidden_dim; i++) {
            float normalized = (x_batch[i] - mean) * inv_std;
            out_batch[i] = gamma[i] * normalized + beta[i];
        }
    }
}
