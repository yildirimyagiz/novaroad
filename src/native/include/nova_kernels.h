#ifndef NOVA_KERNELS_H
#define NOVA_KERNELS_H

#include "nova_tensor.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA UNIFIED KERNEL INTERFACE
 * Cross-platform ML/AI kernel abstraction layer
 * Supports: CPU (SIMD), Metal (Apple), CUDA (NVIDIA), ROCm (AMD)
 * ═══════════════════════════════════════════════════════════════════════════
 */

// ═══════════════════════════════════════════════════════════════════════════
// BACKEND DETECTION & CAPABILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Compile-time backend detection
#if defined(__NVCC__) || defined(__CUDACC__) || defined(NOVA_CUDA_AVAILABLE)
#define NOVA_HAS_CUDA 1
#else
#define NOVA_HAS_CUDA 0
#endif

#if defined(__APPLE__) && (defined(__arm64__) || defined(__aarch64__))
#define NOVA_HAS_METAL 1
#else
#define NOVA_HAS_METAL 0
#endif

#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || defined(_M_ARM64)
#define NOVA_HAS_SIMD 1
#else
#define NOVA_HAS_SIMD 0
#endif

#if defined(__HIP__) || defined(__HIPCC__)
#define NOVA_HAS_ROCM 1
#else
#define NOVA_HAS_ROCM 0
#endif

// Backend capabilities (runtime detection)
typedef struct {
  bool cuda_available;
  bool metal_available;
  bool rocm_available;
  bool simd_available;
  bool cpu_available;
  int cuda_device_count;
  int metal_device_count;
  int rocm_device_count;
} NovaKernelCapabilities;

NovaKernelCapabilities nova_kernel_get_capabilities(void);
void nova_kernel_print_capabilities(void);

// ═══════════════════════════════════════════════════════════════════════════
// KERNEL SUBSYSTEM INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

void nova_kernel_subsystem_init(void);
void nova_kernel_subsystem_cleanup(void);

// Math Kernels
void nova_kernel_matmul(NovaTensor *A, NovaTensor *B, NovaTensor *C);
void nova_kernel_add(NovaTensor *A, NovaTensor *B, NovaTensor *C);
void nova_kernel_mul(NovaTensor *A, NovaTensor *B, NovaTensor *C);
void nova_kernel_scalar_mul(NovaTensor *A, float s, NovaTensor *C);
void nova_kernel_transpose(NovaTensor *A, NovaTensor *C);

// Activation Kernels
void nova_kernel_relu(NovaTensor *x, NovaTensor *out);
void nova_kernel_sigmoid(NovaTensor *x, NovaTensor *out);
void nova_kernel_softmax(NovaTensor *x, int dim, NovaTensor *out);
void nova_kernel_gelu(NovaTensor *x, NovaTensor *out);
void nova_kernel_silu(NovaTensor *x, NovaTensor *out);
void nova_kernel_tanh(NovaTensor *x, NovaTensor *out);
void nova_kernel_clamp(NovaTensor *x, float min, float max, NovaTensor *out);
void nova_kernel_pow(NovaTensor *x, float exponent, NovaTensor *out);
void nova_kernel_unsqueeze(NovaTensor *x, int dim, NovaTensor *out);

// Advanced AI Kernels
void nova_kernel_attention(NovaTensor *Q, NovaTensor *K, NovaTensor *V,
                             NovaTensor *out);
void nova_kernel_conv2d(NovaTensor *input, NovaTensor *weight,
                          NovaTensor *bias, NovaTensor *output, int stride,
                          int padding);

// Stability
void nova_kernel_stabilize_fp(NovaTensor *t);

// Transformer Kernels (NovaTensor-based)
void nova_kernel_layernorm(NovaTensor *x, NovaTensor *gamma,
                             NovaTensor *beta, float eps);
void nova_kernel_matmul_bias_gelu_f32(NovaTensor *A, NovaTensor *B,
                                        NovaTensor *Bias, NovaTensor *Out);

// Fused Kernels
void nova_kernel_matmul_add_relu_f32(NovaTensor *A, NovaTensor *B,
                                       NovaTensor *Bias, NovaTensor *Out);
void nova_kernel_conv2d_bias_relu_f32(NovaTensor *x, NovaTensor *w,
                                        NovaTensor *bias, NovaTensor *out);

// Quantized Operations (Weight-Only)
void nova_kernel_matmul_int8_f16(NovaTensor *A, NovaTensor *W_int8,
                                   NovaTensor *Out_f16);

// Procedural Synthesis Kernels
void nova_kernel_synthetic_pe_f32(NovaTensor *Output);
void nova_kernel_recurrent_pe_f32(NovaTensor *Output);

// Delta Inference Kernel (Experimental)
// Efficiently updates output based on input differences: Out_new = Out_old +
// (Delta * Weights)
void nova_kernel_matmul_delta_sparse_f32(NovaTensor *Delta,
                                           NovaTensor *Weights,
                                           NovaTensor *Out_Start,
                                           float threshold);

// Low-level fused and eager kernels for benchmarks
void nova_conv2d_nchw_f32_no_bias(const float *x, const float *w, float *out,
                                    int B, int C, int H, int W, int OC, int KH,
                                    int KW, int stride, int pad);
void nova_bias_add_nchw_f32(float *data, const float *bias, float *out, int B,
                              int OC, int OH, int OW);
void nova_relu_inplace_f32(float *data, int64_t n);
void nova_conv2d_bias_relu_nchw_f32(const float *x, const float *w,
                                      const float *bias, float *out, int B,
                                      int C, int H, int W, int OC, int KH,
                                      int KW, int stride, int pad);

// ═══════════════════════════════════════════════════════════════════════════
// REDUCTION OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

void nova_kernel_reduce_sum(const NovaTensor *input, NovaTensor *output,
                               const int *dims, int num_dims, bool keepdim);
void nova_kernel_reduce_max(const NovaTensor *input, NovaTensor *output,
                               const int *dims, int num_dims, bool keepdim);
void nova_kernel_reduce_mean(const NovaTensor *input, NovaTensor *output,
                                const int *dims, int num_dims, bool keepdim);

// ═══════════════════════════════════════════════════════════════════════════
// NORMALIZATION OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

void nova_kernel_group_norm(const NovaTensor *input, NovaTensor *output,
                               const NovaTensor *gamma, const NovaTensor *beta,
                               int num_groups, float epsilon);
void nova_kernel_batch_norm(const NovaTensor *input, NovaTensor *output,
                               const NovaTensor *gamma, const NovaTensor *beta,
                               const NovaTensor *running_mean,
                               const NovaTensor *running_var, float epsilon,
                               float momentum, bool training);

#ifdef __cplusplus
}
#endif

#endif // NOVA_KERNELS_H
