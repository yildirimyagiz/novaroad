#include "compute/nova_kernels.h"
#include "ml/nova_tensor.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA FUSED KERNELS - Operation Fusion for Performance
 * ═══════════════════════════════════════════════════════════════════════════
 */

// ═══════════════════════════════════════════════════════════════════════════
// MatMul + Activation Fusions
// ═══════════════════════════════════════════════════════════════════════════

void fused_matmul_gelu(NovaTensor *A, NovaTensor *B, NovaTensor *out) {
    // C = GELU(A @ B)
    // Fused: compute matmul result and apply GELU immediately
    
    int M = A->shape[0];
    int K = A->shape[1];
    int N = B->shape[1];
    
    float *a_data = (float *)A->data;
    float *b_data = (float *)B->data;
    float *out_data = (float *)out->data;
    
    const float sqrt_2_over_pi = 0.7978845608f;
    const float coeff = 0.044715f;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            // Compute matmul element
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += a_data[i * K + k] * b_data[k * N + j];
            }
            
            // Apply GELU immediately (no intermediate storage)
            float x_cubed = sum * sum * sum;
            float inner = sqrt_2_over_pi * (sum + coeff * x_cubed);
            out_data[i * N + j] = 0.5f * sum * (1.0f + tanhf(inner));
        }
    }
}

void fused_matmul_silu(NovaTensor *A, NovaTensor *B, NovaTensor *out) {
    // C = SiLU(A @ B)
    
    int M = A->shape[0];
    int K = A->shape[1];
    int N = B->shape[1];
    
    float *a_data = (float *)A->data;
    float *b_data = (float *)B->data;
    float *out_data = (float *)out->data;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += a_data[i * K + k] * b_data[k * N + j];
            }
            
            // Apply SiLU: x * sigmoid(x)
            float sigmoid = 1.0f / (1.0f + expf(-sum));
            out_data[i * N + j] = sum * sigmoid;
        }
    }
}

void fused_matmul_relu(NovaTensor *A, NovaTensor *B, NovaTensor *out) {
    // C = ReLU(A @ B)
    
    int M = A->shape[0];
    int K = A->shape[1];
    int N = B->shape[1];
    
    float *a_data = (float *)A->data;
    float *b_data = (float *)B->data;
    float *out_data = (float *)out->data;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += a_data[i * K + k] * b_data[k * N + j];
            }
            
            out_data[i * N + j] = sum > 0.0f ? sum : 0.0f;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// MatMul + Bias + Activation
// ═══════════════════════════════════════════════════════════════════════════

void fused_matmul_bias_gelu(NovaTensor *A, NovaTensor *B, NovaTensor *bias, 
                            NovaTensor *out) {
    // C = GELU(A @ B + bias)
    
    int M = A->shape[0];
    int K = A->shape[1];
    int N = B->shape[1];
    
    float *a_data = (float *)A->data;
    float *b_data = (float *)B->data;
    float *bias_data = (float *)bias->data;
    float *out_data = (float *)out->data;
    
    const float sqrt_2_over_pi = 0.7978845608f;
    const float coeff = 0.044715f;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += a_data[i * K + k] * b_data[k * N + j];
            }
            
            // Add bias
            sum += bias_data[j];
            
            // Apply GELU
            float x_cubed = sum * sum * sum;
            float inner = sqrt_2_over_pi * (sum + coeff * x_cubed);
            out_data[i * N + j] = 0.5f * sum * (1.0f + tanhf(inner));
        }
    }
}

void fused_matmul_bias_relu(NovaTensor *A, NovaTensor *B, NovaTensor *bias,
                            NovaTensor *out) {
    // C = ReLU(A @ B + bias)
    
    int M = A->shape[0];
    int K = A->shape[1];
    int N = B->shape[1];
    
    float *a_data = (float *)A->data;
    float *b_data = (float *)B->data;
    float *bias_data = (float *)bias->data;
    float *out_data = (float *)out->data;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = bias_data[j];
            for (int k = 0; k < K; k++) {
                sum += a_data[i * K + k] * b_data[k * N + j];
            }
            
            out_data[i * N + j] = sum > 0.0f ? sum : 0.0f;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// LayerNorm + Activation Fusion
// ═══════════════════════════════════════════════════════════════════════════

void fused_layernorm_gelu(NovaTensor *x, NovaTensor *gamma, NovaTensor *beta,
                          NovaTensor *out, float eps) {
    // out = GELU(LayerNorm(x))
    
    int batch = x->shape[0];
    int dim = x->shape[1];
    
    float *x_data = (float *)x->data;
    float *gamma_data = (float *)gamma->data;
    float *beta_data = (float *)beta->data;
    float *out_data = (float *)out->data;
    
    const float sqrt_2_over_pi = 0.7978845608f;
    const float coeff = 0.044715f;
    
    for (int b = 0; b < batch; b++) {
        float *x_row = &x_data[b * dim];
        float *out_row = &out_data[b * dim];
        
        // Compute mean
        float mean = 0.0f;
        for (int i = 0; i < dim; i++) {
            mean += x_row[i];
        }
        mean /= dim;
        
        // Compute variance
        float var = 0.0f;
        for (int i = 0; i < dim; i++) {
            float diff = x_row[i] - mean;
            var += diff * diff;
        }
        var /= dim;
        
        // Normalize and apply GELU
        float rstd = 1.0f / sqrtf(var + eps);
        for (int i = 0; i < dim; i++) {
            float normalized = (x_row[i] - mean) * rstd;
            float scaled = normalized * gamma_data[i] + beta_data[i];
            
            // Apply GELU
            float x_cubed = scaled * scaled * scaled;
            float inner = sqrt_2_over_pi * (scaled + coeff * x_cubed);
            out_row[i] = 0.5f * scaled * (1.0f + tanhf(inner));
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Attention Score Fusion (Q @ K^T / sqrt(d) + mask)
// ═══════════════════════════════════════════════════════════════════════════

void fused_attention_score(NovaTensor *Q, NovaTensor *K, NovaTensor *scores,
                           float scale, NovaTensor *mask) {
    // scores = (Q @ K^T) / sqrt(d_k) + mask
    // Then softmax (handled separately or can be fused too)
    
    int batch = Q->shape[0];
    int seq_len = Q->shape[1];
    int d_k = Q->shape[2];
    
    float *q_data = (float *)Q->data;
    float *k_data = (float *)K->data;
    float *scores_data = (float *)scores->data;
    float *mask_data = mask ? (float *)mask->data : NULL;
    
    for (int b = 0; b < batch; b++) {
        for (int i = 0; i < seq_len; i++) {
            for (int j = 0; j < seq_len; j++) {
                // Q[i] @ K[j]^T
                float dot = 0.0f;
                for (int k = 0; k < d_k; k++) {
                    int q_idx = b * seq_len * d_k + i * d_k + k;
                    int k_idx = b * seq_len * d_k + j * d_k + k;
                    dot += q_data[q_idx] * k_data[k_idx];
                }
                
                // Scale
                dot *= scale;
                
                // Add mask
                if (mask_data) {
                    int mask_idx = b * seq_len * seq_len + i * seq_len + j;
                    dot += mask_data[mask_idx];
                }
                
                int out_idx = b * seq_len * seq_len + i * seq_len + j;
                scores_data[out_idx] = dot;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Element-wise Fusions
// ═══════════════════════════════════════════════════════════════════════════

void fused_add_relu(NovaTensor *a, NovaTensor *b, NovaTensor *out) {
    // out = ReLU(a + b)
    
    float *a_data = (float *)a->data;
    float *b_data = (float *)b->data;
    float *out_data = (float *)out->data;
    size_t n = a->total_elements;
    
    for (size_t i = 0; i < n; i++) {
        float sum = a_data[i] + b_data[i];
        out_data[i] = sum > 0.0f ? sum : 0.0f;
    }
}

void fused_mul_sigmoid(NovaTensor *a, NovaTensor *b, NovaTensor *out) {
    // out = sigmoid(a * b)
    
    float *a_data = (float *)a->data;
    float *b_data = (float *)b->data;
    float *out_data = (float *)out->data;
    size_t n = a->total_elements;
    
    for (size_t i = 0; i < n; i++) {
        float prod = a_data[i] * b_data[i];
        out_data[i] = 1.0f / (1.0f + expf(-prod));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Performance Comparison Helper
// ═══════════════════════════════════════════════════════════════════════════

void benchmark_fused_vs_separate(NovaTensor *A, NovaTensor *B) {
    (void)A;
    (void)B;
    printf("🔬 Benchmark stub - link with full runtime for benchmarking\n");
    // Note: This function requires linking with nova_tensor.c
    // Use external benchmark_kernels.c for full benchmarking
}
