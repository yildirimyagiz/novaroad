/**
 * @file tensor_advanced.h
 * @brief Advanced tensor operations (FP16, GPU, multi-threading)
 */

#ifndef NOVA_TENSOR_ADVANCED_H
#define NOVA_TENSOR_ADVANCED_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Multi-threading ========== */

/**
 * Multi-threaded matrix multiplication
 * @param num_threads Number of threads (0 = auto-detect)
 * @return 0 on success, -1 on error
 */
int nova_tensor_matmul_threaded(
    const float *A,
    const float *B,
    float *C,
    size_t M, size_t N, size_t K,
    int num_threads);

/* ========== Float16 (FP16) Operations ========== */

#ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
typedef __fp16 float16_t;
#else
typedef uint16_t float16_t;
#endif

/**
 * FP16 matrix multiplication (2x throughput)
 * @return 0 on success, -1 on error or not supported
 */
int nova_fp16_matmul(
    const float16_t *A,
    const float16_t *B,
    float16_t *C,
    size_t M, size_t N, size_t K);

/**
 * Convert FP32 to FP16
 */
void nova_fp32_to_fp16(const float *src, float16_t *dst, size_t n);

/**
 * Convert FP16 to FP32
 */
void nova_fp16_to_fp32(const float16_t *src, float *dst, size_t n);

/**
 * FP16 element-wise operations
 */
void nova_fp16_add(const float16_t *a, const float16_t *b, float16_t *c, size_t n);
void nova_fp16_mul(const float16_t *a, const float16_t *b, float16_t *c, size_t n);
void nova_fp16_relu(const float16_t *x, float16_t *out, size_t n);

/* ========== GPU Acceleration (Metal) ========== */

/**
 * Initialize Metal GPU backend
 * @return 0 on success, -1 if Metal not available
 */
int nova_metal_gpu_init(void);

/**
 * Cleanup Metal GPU backend
 */
void nova_metal_gpu_cleanup(void);

/**
 * GPU-accelerated matrix multiplication
 * Automatically selects best kernel based on size
 * Expected: 1000-2600 GFLOPS on M1 GPU (100-300x speedup for large matrices)
 */
int nova_metal_gpu_gemm(
    const float *A,
    const float *B,
    float *C,
    int M, int N, int K);

/**
 * GPU FP16 GEMM (2x faster)
 */
int nova_metal_gpu_gemm_fp16(
    const float16_t *A,
    const float16_t *B,
    float16_t *C,
    int M, int N, int K);

/* ========== Fused Kernels ========== */

/**
 * Fused: MatMul + Bias + ReLU (single pass, 2-3x faster)
 */
int nova_fused_linear_relu(
    const float *input,
    const float *weight,
    const float *bias,
    float *output,
    size_t batch, size_t in_features, size_t out_features,
    int transpose_weight);

/**
 * Fused: Add + ReLU (inplace)
 */
void nova_fused_add_relu_inplace(
    float *a,
    const float *b,
    size_t n);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_TENSOR_ADVANCED_H */
