/**
 * @file blas_backend.c
 * @brief BLAS/LAPACK integration for optimized linear algebra
 */

#include "ai/tensor.h"
#include <stddef.h>
#include <string.h>

/* BLAS interface (can link to OpenBLAS, MKL, Accelerate, etc.) */

/* Matrix multiplication: C = alpha*A*B + beta*C */
void nova_tensor_gemm_blas(
    bool trans_a, bool trans_b,
    size_t m, size_t n, size_t k,
    float alpha,
    const float *A, size_t lda,
    const float *B, size_t ldb,
    float beta,
    float *C, size_t ldc)
{
#ifdef NOVA_USE_BLAS
    /* Call BLAS sgemm if available */
    extern void sgemm_(const char *transa, const char *transb,
                      const int *m, const int *n, const int *k,
                      const float *alpha,
                      const float *a, const int *lda,
                      const float *b, const int *ldb,
                      const float *beta,
                      float *c, const int *ldc);
    
    char ta = trans_a ? 'T' : 'N';
    char tb = trans_b ? 'T' : 'N';
    int im = (int)m, in = (int)n, ik = (int)k;
    int ilda = (int)lda, ildb = (int)ldb, ildc = (int)ldc;
    
    sgemm_(&ta, &tb, &im, &in, &ik, &alpha, A, &ilda, B, &ildb, &beta, C, &ildc);
#else
    /* Fallback: naive implementation */
    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n; j++) {
            float sum = 0.0f;
            for (size_t p = 0; p < k; p++) {
                float a_val = trans_a ? A[p * lda + i] : A[i * lda + p];
                float b_val = trans_b ? B[j * ldb + p] : B[p * ldb + j];
                sum += a_val * b_val;
            }
            C[i * ldc + j] = alpha * sum + beta * C[i * ldc + j];
        }
    }
#endif
}

/* Batched matrix multiplication */
void nova_tensor_gemm_batch(
    size_t batch_size,
    bool trans_a, bool trans_b,
    size_t m, size_t n, size_t k,
    float alpha,
    const float **A_array, size_t lda,
    const float **B_array, size_t ldb,
    float beta,
    float **C_array, size_t ldc)
{
    for (size_t b = 0; b < batch_size; b++) {
        nova_tensor_gemm_blas(trans_a, trans_b, m, n, k,
                             alpha, A_array[b], lda,
                             B_array[b], ldb,
                             beta, C_array[b], ldc);
    }
}
