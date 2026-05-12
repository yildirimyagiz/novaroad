/**
 * @file cross_platform.h
 * @brief Cross-platform optimizations API
 * 
 * Unified interface for all platforms:
 * - x86_64: AVX2, AVX-512
 * - ARM64: NEON, FP16, DotProd
 * - NVIDIA: CUDA, Tensor Cores
 * - AMD: ROCm, WMMA
 * - Distributed: MPI
 */

#ifndef NOVA_CROSS_PLATFORM_H
#define NOVA_CROSS_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== x86_64 SIMD ========== */

/**
 * AVX2 GEMM (Intel Haswell+, AMD Zen+)
 * Performance: 200-400 GFLOPS
 */
int nova_sgemm_avx2(
    const float* A, const float* B, float* C,
    size_t M, size_t N, size_t K);

/**
 * AVX-512 GEMM (Intel Skylake-X+, Sapphire Rapids)
 * Performance: 400-800 GFLOPS
 */
int nova_sgemm_avx512(
    const float* A, const float* B, float* C,
    size_t M, size_t N, size_t K);

/* ========== CUDA Backend ========== */

/**
 * Initialize CUDA
 */
int nova_cuda_init(void);
void nova_cuda_cleanup(void);

/**
 * CUDA GEMM using cuBLAS
 * Performance: 5-40 TFLOPS (RTX 3000/4000, A100, H100)
 */
int nova_cuda_gemm(
    const float* A, const float* B, float* C,
    int M, int N, int K);

/**
 * Fused CUDA kernel: GEMM + Bias + ReLU
 */
int nova_cuda_gemm_bias_relu(
    const float* A, const float* B, const float* bias, float* C,
    int M, int N, int K);

/**
 * Tensor Core GEMM (mixed precision)
 * Performance: 2-4× faster than FP32
 */
int nova_cuda_gemm_tensorcore(
    const float* A, const float* B, float* C,
    int M, int N, int K);

/* ========== ROCm Backend ========== */

/**
 * Initialize ROCm
 */
int nova_rocm_init(void);
void nova_rocm_cleanup(void);

/**
 * ROCm GEMM using rocBLAS
 * Performance: 10-50 TFLOPS (MI100/MI200/MI300)
 */
int nova_rocm_gemm(
    const float* A, const float* B, float* C,
    int M, int N, int K);

/**
 * Fused HIP kernel: GEMM + Bias + ReLU
 */
int nova_rocm_gemm_bias_relu(
    const float* A, const float* B, const float* bias, float* C,
    int M, int N, int K);

/* ========== Int8 Quantization ========== */

typedef struct {
    float scale;
    int8_t zero_point;
} QuantParams;

/**
 * Quantize FP32 to INT8
 * 4× less memory, 4× higher throughput
 */
void nova_quantize_fp32_to_int8(
    const float* input,
    int8_t* output,
    size_t n,
    QuantParams* params);

/**
 * Dequantize INT8 to FP32
 */
void nova_dequantize_int8_to_fp32(
    const int8_t* input,
    float* output,
    size_t n,
    const QuantParams* params);

/**
 * Quantized GEMM (INT8 compute, FP32 I/O)
 * Performance: 4× faster than FP32
 */
int nova_quantized_matmul(
    const float* A_fp32,
    const float* B_fp32,
    float* C_fp32,
    size_t M, size_t N, size_t K);

/* ========== Sparse Matrices ========== */

typedef struct {
    float* values;
    int* col_indices;
    int* row_ptrs;
    int num_rows;
    int num_cols;
    int nnz;
} CSRMatrix;

/**
 * Convert dense to sparse (CSR format)
 */
CSRMatrix* nova_dense_to_csr(const float* dense, int rows, int cols);

/**
 * Free sparse matrix
 */
void nova_csr_free(CSRMatrix* csr);

/**
 * Sparse matrix × dense vector
 * Speedup: (rows × cols) / nnz (up to 100×!)
 */
void nova_spmv_csr(
    const CSRMatrix* A,
    const float* x,
    float* y);

/**
 * Sparse matrix × dense matrix
 */
void nova_spgemm_dense(
    const CSRMatrix* A,
    const float* B,
    float* C,
    int N);

/**
 * Sparse × sparse matrix multiplication
 */
CSRMatrix* nova_spgemm_sparse(
    const CSRMatrix* A,
    const CSRMatrix* B);

/* ========== MPI Distributed Computing ========== */

/**
 * Initialize MPI
 */
int nova_mpi_init(int* argc, char*** argv);
void nova_mpi_finalize(void);

/**
 * Get MPI rank and size
 */
int nova_mpi_get_rank(void);
int nova_mpi_get_size(void);

/**
 * Distributed GEMM (data parallelism)
 * Speedup: ~linear with number of nodes
 */
int nova_mpi_gemm_data_parallel(
    const float* A, const float* B, float* C,
    int M, int N, int K,
    int (*gemm_fn)(const float*, const float*, float*, int, int, int));

/**
 * All-reduce for gradient synchronization
 */
int nova_mpi_allreduce_gradients(float* gradients, int N);

/**
 * Broadcast parameters
 */
int nova_mpi_broadcast_params(float* params, int N, int root);

/**
 * Ring all-reduce (bandwidth-optimal)
 */
int nova_mpi_ring_allreduce(
    const float* send_buf,
    float* recv_buf,
    int N);

/* ========== Auto-dispatch ========== */

/**
 * Automatically select best GEMM implementation
 * Based on: matrix size, available hardware, data type
 */
int nova_gemm_auto(
    const float* A, const float* B, float* C,
    int M, int N, int K);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_CROSS_PLATFORM_H */
