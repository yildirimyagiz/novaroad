#ifndef NOVA_KERNELS_ADVANCED_H
#define NOVA_KERNELS_ADVANCED_H

#include "../ml/nova_tensor.h"
#include "nova_kernels.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ADVANCED KERNELS - GPU, SIMD & Fused Operations
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════
// CUDA Kernels (GPU)
// ═══════════════════════════════════════════════════════════════════════════

#ifdef USE_CUDA
void nova_cuda_gelu(NovaTensor *x, NovaTensor *out);
void nova_cuda_silu(NovaTensor *x, NovaTensor *out);
void nova_cuda_relu(NovaTensor *x, NovaTensor *out);
void nova_cuda_sigmoid(NovaTensor *x, NovaTensor *out);
void nova_cuda_tanh(NovaTensor *x, NovaTensor *out);
void nova_cuda_softmax(NovaTensor *x, NovaTensor *out);
void nova_cuda_matmul(NovaTensor *A, NovaTensor *B, NovaTensor *C);
void nova_cuda_add(NovaTensor *a, NovaTensor *b, NovaTensor *c);
void nova_cuda_mul(NovaTensor *a, NovaTensor *b, NovaTensor *c);
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SIMD Kernels (AVX2/AVX-512)
// ═══════════════════════════════════════════════════════════════════════════

void nova_simd_gelu(NovaTensor *x, NovaTensor *out);
void nova_simd_silu(NovaTensor *x, NovaTensor *out);
void nova_simd_relu(NovaTensor *x, NovaTensor *out);
void nova_simd_add(NovaTensor *a, NovaTensor *b, NovaTensor *c);
void nova_simd_mul(NovaTensor *a, NovaTensor *b, NovaTensor *c);

// Low-level SIMD kernels
#ifdef __AVX2__
void simd_kernel_relu_avx2(const float *x, float *out, size_t n);
void simd_kernel_gelu_avx2(const float *x, float *out, size_t n);
void simd_kernel_silu_avx2(const float *x, float *out, size_t n);
void simd_kernel_add_avx2(const float *a, const float *b, float *c, size_t n);
void simd_kernel_mul_avx2(const float *a, const float *b, float *c, size_t n);
#endif

#ifdef __AVX512F__
void simd_kernel_relu_avx512(const float *x, float *out, size_t n);
void simd_kernel_add_avx512(const float *a, const float *b, float *c, size_t n);
void simd_kernel_mul_avx512(const float *a, const float *b, float *c, size_t n);
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Fused Kernels (Operation Fusion)
// ═══════════════════════════════════════════════════════════════════════════

// MatMul + Activation
void fused_matmul_gelu(NovaTensor *A, NovaTensor *B, NovaTensor *out);
void fused_matmul_silu(NovaTensor *A, NovaTensor *B, NovaTensor *out);
void fused_matmul_relu(NovaTensor *A, NovaTensor *B, NovaTensor *out);

// MatMul + Bias + Activation
void fused_matmul_bias_gelu(NovaTensor *A, NovaTensor *B, NovaTensor *bias, NovaTensor *out);
void fused_matmul_bias_relu(NovaTensor *A, NovaTensor *B, NovaTensor *bias, NovaTensor *out);

// LayerNorm + Activation
void fused_layernorm_gelu(NovaTensor *x, NovaTensor *gamma, NovaTensor *beta,
                          NovaTensor *out, float eps);

// Attention Operations
void fused_attention_score(NovaTensor *Q, NovaTensor *K, NovaTensor *scores,
                           float scale, NovaTensor *mask);

// Element-wise Fusions
void fused_add_relu(NovaTensor *a, NovaTensor *b, NovaTensor *out);
void fused_mul_sigmoid(NovaTensor *a, NovaTensor *b, NovaTensor *out);

// Benchmarking
void benchmark_fused_vs_separate(NovaTensor *A, NovaTensor *B);

// ═══════════════════════════════════════════════════════════════════════════
// Smart Dispatcher (Auto Backend Selection)
// ═══════════════════════════════════════════════════════════════════════════

void nova_dispatcher_init(void);

// Auto-dispatch operations
void nova_dispatch_gelu(NovaTensor *x, NovaTensor *out);
void nova_dispatch_silu(NovaTensor *x, NovaTensor *out);
void nova_dispatch_relu(NovaTensor *x, NovaTensor *out);
void nova_dispatch_matmul(NovaTensor *A, NovaTensor *B, NovaTensor *C);
void nova_dispatch_add(NovaTensor *a, NovaTensor *b, NovaTensor *c);

// Fused dispatches
void nova_dispatch_matmul_gelu(NovaTensor *A, NovaTensor *B, NovaTensor *out);
void nova_dispatch_matmul_silu(NovaTensor *A, NovaTensor *B, NovaTensor *out);
void nova_dispatch_matmul_bias_gelu(NovaTensor *A, NovaTensor *B, 
                                      NovaTensor *bias, NovaTensor *out);

// Statistics
void nova_dispatcher_print_stats(void);
void nova_dispatcher_reset_stats(void);

// ═══════════════════════════════════════════════════════════════════════════
// Feature Detection
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int has_cuda;
    int has_avx2;
    int has_avx512;
    int cuda_device_count;
    char cuda_device_name[256];
} NovaHardwareInfo;

void nova_get_hardware_info(NovaHardwareInfo *info);

#ifdef __cplusplus
}
#endif

#endif // NOVA_KERNELS_ADVANCED_H
