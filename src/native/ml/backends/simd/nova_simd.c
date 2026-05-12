/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_simd.c - High-Performance Vectorized Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_simd.h"
#include <stdio.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// VECTOR ADDITION
// ═══════════════════════════════════════════════════════════════════════════

void nova_simd_vec_add_f32(const float *a, const float *b, float *c,
                             size_t n) {
#if defined(NOVA_SIMD_AVX512)
  size_t i = 0;
  for (; i + 16 <= n; i += 16) {
    nova_simd_store_f32(c + i,
                          nova_simd_add_f32(nova_simd_load_f32(a + i),
                                              nova_simd_load_f32(b + i)));
  }
  for (; i < n; i++)
    c[i] = a[i] + b[i];

#elif defined(NOVA_SIMD_AVX2)
  size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    nova_simd_store_f32(c + i,
                          nova_simd_add_f32(nova_simd_load_f32(a + i),
                                              nova_simd_load_f32(b + i)));
  }
  for (; i < n; i++)
    c[i] = a[i] + b[i];

#elif defined(NOVA_SIMD_NEON)
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    nova_simd_store_f32(c + i,
                          nova_simd_add_f32(nova_simd_load_f32(a + i),
                                              nova_simd_load_f32(b + i)));
  }
  for (; i < n; i++)
    c[i] = a[i] + b[i];

#else
  for (size_t i = 0; i < n; i++)
    c[i] = a[i] + b[i];
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// VECTOR MULTIPLICATION
// ═══════════════════════════════════════════════════════════════════════════

void nova_simd_vec_mul_f32(const float *a, const float *b, float *c,
                             size_t n) {
#if defined(NOVA_SIMD_AVX512)
  size_t i = 0;
  for (; i + 16 <= n; i += 16) {
    NOVA_PREFETCH_READ(a + i + 16);
    NOVA_PREFETCH_READ(b + i + 16);
    nova_simd_store_f32(c + i,
                          nova_simd_mul_f32(nova_simd_load_f32(a + i),
                                              nova_simd_load_f32(b + i)));
  }
  for (; i < n; i++)
    c[i] = a[i] * b[i];

#elif defined(NOVA_SIMD_AVX2)
  size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    nova_simd_store_f32(c + i,
                          nova_simd_mul_f32(nova_simd_load_f32(a + i),
                                              nova_simd_load_f32(b + i)));
  }
  for (; i < n; i++)
    c[i] = a[i] * b[i];

#elif defined(NOVA_SIMD_NEON)
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    nova_simd_store_f32(c + i,
                          nova_simd_mul_f32(nova_simd_load_f32(a + i),
                                              nova_simd_load_f32(b + i)));
  }
  for (; i < n; i++)
    c[i] = a[i] * b[i];

#else
  for (size_t i = 0; i < n; i++)
    c[i] = a[i] * b[i];
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// DOT PRODUCT
// ═══════════════════════════════════════════════════════════════════════════

float nova_simd_dot_product_f32(const float *a, const float *b, size_t n) {
  float sum = 0.0f;

#if defined(NOVA_SIMD_AVX512)
  size_t i = 0;
  nova_vec16f vsum0 = _mm512_setzero_ps();
  nova_vec16f vsum1 = _mm512_setzero_ps();
  nova_vec16f vsum2 = _mm512_setzero_ps();
  nova_vec16f vsum3 = _mm512_setzero_ps();

  for (; i + 64 <= n; i += 64) {
    vsum0 = nova_simd_fma_f32(nova_simd_load_f32(a + i),
                                nova_simd_load_f32(b + i), vsum0);
    vsum1 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 16),
                                nova_simd_load_f32(b + i + 16), vsum1);
    vsum2 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 32),
                                nova_simd_load_f32(b + i + 32), vsum2);
    vsum3 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 48),
                                nova_simd_load_f32(b + i + 48), vsum3);
  }

  nova_vec16f vtotal =
      _mm512_add_ps(_mm512_add_ps(vsum0, vsum1), _mm512_add_ps(vsum2, vsum3));

  for (; i + 16 <= n; i += 16) {
    vtotal = nova_simd_fma_f32(nova_simd_load_f32(a + i),
                                 nova_simd_load_f32(b + i), vtotal);
  }

  sum = nova_simd_reduce_sum_f32(vtotal);
  for (; i < n; i++)
    sum += a[i] * b[i];

#elif defined(NOVA_SIMD_AVX2)
  size_t i = 0;
  nova_vec8f vsum0 = _mm256_setzero_ps();
  nova_vec8f vsum1 = _mm256_setzero_ps();
  nova_vec8f vsum2 = _mm256_setzero_ps();
  nova_vec8f vsum3 = _mm256_setzero_ps();

  for (; i + 32 <= n; i += 32) {
    vsum0 = nova_simd_fma_f32(nova_simd_load_f32(a + i),
                                nova_simd_load_f32(b + i), vsum0);
    vsum1 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 8),
                                nova_simd_load_f32(b + i + 8), vsum1);
    vsum2 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 16),
                                nova_simd_load_f32(b + i + 16), vsum2);
    vsum3 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 24),
                                nova_simd_load_f32(b + i + 24), vsum3);
  }

  nova_vec8f vtotal =
      _mm256_add_ps(_mm256_add_ps(vsum0, vsum1), _mm256_add_ps(vsum2, vsum3));

  for (; i + 8 <= n; i += 8) {
    vtotal = nova_simd_fma_f32(nova_simd_load_f32(a + i),
                                 nova_simd_load_f32(b + i), vtotal);
  }

  sum = nova_simd_reduce_sum_f32(vtotal);
  for (; i < n; i++)
    sum += a[i] * b[i];

#elif defined(NOVA_SIMD_NEON)
  size_t i = 0;
  nova_vec4f vsum0 = vdupq_n_f32(0.0f);
  nova_vec4f vsum1 = vdupq_n_f32(0.0f);
  nova_vec4f vsum2 = vdupq_n_f32(0.0f);
  nova_vec4f vsum3 = vdupq_n_f32(0.0f);

  for (; i + 16 <= n; i += 16) {
    vsum0 = nova_simd_fma_f32(nova_simd_load_f32(a + i),
                                nova_simd_load_f32(b + i), vsum0);
    vsum1 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 4),
                                nova_simd_load_f32(b + i + 4), vsum1);
    vsum2 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 8),
                                nova_simd_load_f32(b + i + 8), vsum2);
    vsum3 = nova_simd_fma_f32(nova_simd_load_f32(a + i + 12),
                                nova_simd_load_f32(b + i + 12), vsum3);
  }

  nova_vec4f vtotal =
      vaddq_f32(vaddq_f32(vsum0, vsum1), vaddq_f32(vsum2, vsum3));

  for (; i + 4 <= n; i += 4) {
    vtotal = nova_simd_fma_f32(nova_simd_load_f32(a + i),
                                 nova_simd_load_f32(b + i), vtotal);
  }

  sum = nova_simd_reduce_sum_f32(vtotal);
  for (; i < n; i++)
    sum += a[i] * b[i];

#else
  for (size_t i = 0; i < n; i++)
    sum += a[i] * b[i];
#endif

  return sum;
}

// ═══════════════════════════════════════════════════════════════════════════
// MATRIX MULTIPLICATION (Blocked + SIMD)
// ═══════════════════════════════════════════════════════════════════════════

void nova_simd_matmul_f32(const float *A, const float *B, float *C, size_t M,
                            size_t N, size_t K) {
  memset(C, 0, M * N * sizeof(float));
  const size_t BLOCK_SIZE = 64;

  for (size_t i = 0; i < M; i += BLOCK_SIZE) {
    for (size_t k = 0; k < K; k += BLOCK_SIZE) {
      for (size_t j = 0; j < N; j += BLOCK_SIZE) {

        size_t i_end = (i + BLOCK_SIZE > M) ? M : i + BLOCK_SIZE;
        size_t k_end = (k + BLOCK_SIZE > K) ? K : k + BLOCK_SIZE;
        size_t j_end = (j + BLOCK_SIZE > N) ? N : j + BLOCK_SIZE;

        for (size_t ii = i; ii < i_end; ii++) {
          for (size_t kk = k; kk < k_end; kk++) {
            float aik = A[ii * K + kk];

#if defined(NOVA_SIMD_AVX512)
            size_t jj = j;
            nova_vec16f vaik = _mm512_set1_ps(aik);
            for (; jj + 64 <= j_end; jj += 64) {
              nova_simd_store_f32(
                  C + ii * N + jj,
                  nova_simd_fma_f32(vaik,
                                      nova_simd_load_f32(B + kk * N + jj),
                                      nova_simd_load_f32(C + ii * N + jj)));
              nova_simd_store_f32(
                  C + ii * N + jj + 16,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 16),
                      nova_simd_load_f32(C + ii * N + jj + 16)));
              nova_simd_store_f32(
                  C + ii * N + jj + 32,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 32),
                      nova_simd_load_f32(C + ii * N + jj + 32)));
              nova_simd_store_f32(
                  C + ii * N + jj + 48,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 48),
                      nova_simd_load_f32(C + ii * N + jj + 48)));
            }
            for (; jj + 16 <= j_end; jj += 16) {
              nova_simd_store_f32(
                  C + ii * N + jj,
                  nova_simd_fma_f32(vaik,
                                      nova_simd_load_f32(B + kk * N + jj),
                                      nova_simd_load_f32(C + ii * N + jj)));
            }
            for (; jj < j_end; jj++)
              C[ii * N + jj] += aik * B[kk * N + jj];
#elif defined(NOVA_SIMD_AVX2)
            size_t jj = j;
            nova_vec8f vaik = _mm256_set1_ps(aik);
            for (; jj + 32 <= j_end; jj += 32) {
              nova_simd_store_f32(
                  C + ii * N + jj,
                  nova_simd_fma_f32(vaik,
                                      nova_simd_load_f32(B + kk * N + jj),
                                      nova_simd_load_f32(C + ii * N + jj)));
              nova_simd_store_f32(
                  C + ii * N + jj + 8,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 8),
                      nova_simd_load_f32(C + ii * N + jj + 8)));
              nova_simd_store_f32(
                  C + ii * N + jj + 16,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 16),
                      nova_simd_load_f32(C + ii * N + jj + 16)));
              nova_simd_store_f32(
                  C + ii * N + jj + 24,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 24),
                      nova_simd_load_f32(C + ii * N + jj + 24)));
            }
            for (; jj + 8 <= j_end; jj += 8) {
              nova_simd_store_f32(
                  C + ii * N + jj,
                  nova_simd_fma_f32(vaik,
                                      nova_simd_load_f32(B + kk * N + jj),
                                      nova_simd_load_f32(C + ii * N + jj)));
            }
            for (; jj < j_end; jj++)
              C[ii * N + jj] += aik * B[kk * N + jj];
#elif defined(NOVA_SIMD_NEON)
            size_t jj = j;
            nova_vec4f vaik = vdupq_n_f32(aik);
            for (; jj + 16 <= j_end; jj += 16) {
              nova_simd_store_f32(
                  C + ii * N + jj,
                  nova_simd_fma_f32(vaik,
                                      nova_simd_load_f32(B + kk * N + jj),
                                      nova_simd_load_f32(C + ii * N + jj)));
              nova_simd_store_f32(
                  C + ii * N + jj + 4,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 4),
                      nova_simd_load_f32(C + ii * N + jj + 4)));
              nova_simd_store_f32(
                  C + ii * N + jj + 8,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 8),
                      nova_simd_load_f32(C + ii * N + jj + 8)));
              nova_simd_store_f32(
                  C + ii * N + jj + 12,
                  nova_simd_fma_f32(
                      vaik, nova_simd_load_f32(B + kk * N + jj + 12),
                      nova_simd_load_f32(C + ii * N + jj + 12)));
            }
            for (; jj + 4 <= j_end; jj += 4) {
              nova_simd_store_f32(
                  C + ii * N + jj,
                  nova_simd_fma_f32(vaik,
                                      nova_simd_load_f32(B + kk * N + jj),
                                      nova_simd_load_f32(C + ii * N + jj)));
            }
            for (; jj < j_end; jj++)
              C[ii * N + jj] += aik * B[kk * N + jj];
#else
            for (size_t jj = j; jj < j_end; jj++)
              C[ii * N + jj] += aik * B[kk * N + jj];
#endif
          }
        }
      }
    }
  }
}
