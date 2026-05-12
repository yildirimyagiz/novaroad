/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_simd.h - SIMD Vectorization (Mojo'nun Ana Gücü)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_SIMD_H
#define NOVA_SIMD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Platform detection
#if defined(__x86_64__) || defined(_M_X64)
#if defined(__AVX512F__)
#define NOVA_SIMD_AVX512 1
#define NOVA_SIMD_WIDTH 512
#include <immintrin.h>
typedef __m512 nova_vec16f;
typedef __m512i nova_vec16i;
typedef __m512d nova_vec8d;
#elif defined(__AVX2__)
#define NOVA_SIMD_AVX2 1
#define NOVA_SIMD_WIDTH 256
#include <immintrin.h>
typedef __m256 nova_vec8f;
typedef __m256i nova_vec8i;
typedef __m256d nova_vec4d;
#elif defined(__SSE4_2__)
#define NOVA_SIMD_SSE42 1
#define NOVA_SIMD_WIDTH 128
#include <smmintrin.h>
typedef __m128 nova_vec4f;
typedef __m128i nova_vec4i;
typedef __m128d nova_vec2d;
#endif
#elif defined(__aarch64__)
#define NOVA_SIMD_NEON 1
#define NOVA_SIMD_WIDTH 128
#include <arm_neon.h>
typedef float32x4_t nova_vec4f;
typedef int32x4_t nova_vec4i;
typedef float64x2_t nova_vec2d;
#endif

// Auto-vectorization pragmas
#define NOVA_VECTORIZE                                                       \
  _Pragma("clang loop vectorize(enable) interleave(enable)")
#define NOVA_NO_VECTORIZE _Pragma("clang loop vectorize(disable)")
#define NOVA_UNROLL(n) _Pragma("clang loop unroll_count(n)")
#define NOVA_ASSUME_ALIGNED(ptr, align) __builtin_assume_aligned(ptr, align)

// Vector operations (unified API across platforms)
#ifdef NOVA_SIMD_AVX512
// 16-wide float operations
static inline nova_vec16f nova_simd_add_f32(nova_vec16f a,
                                                nova_vec16f b) {
  return _mm512_add_ps(a, b);
}
static inline nova_vec16f nova_simd_mul_f32(nova_vec16f a,
                                                nova_vec16f b) {
  return _mm512_mul_ps(a, b);
}
static inline nova_vec16f
nova_simd_fma_f32(nova_vec16f a, nova_vec16f b, nova_vec16f c) {
  return _mm512_fmadd_ps(a, b, c); // a*b + c
}
static inline nova_vec16f nova_simd_load_f32(const float *ptr) {
  return _mm512_load_ps(ptr);
}
static inline void nova_simd_store_f32(float *ptr, nova_vec16f v) {
  _mm512_store_ps(ptr, v);
}
static inline float nova_simd_reduce_sum_f32(nova_vec16f v) {
  return _mm512_reduce_add_ps(v);
}
#elif defined(NOVA_SIMD_NEON)
// NEON operations
static inline nova_vec4f nova_simd_add_f32(nova_vec4f a, nova_vec4f b) {
  return vaddq_f32(a, b);
}
static inline nova_vec4f nova_simd_mul_f32(nova_vec4f a, nova_vec4f b) {
  return vmulq_f32(a, b);
}
static inline nova_vec4f nova_simd_fma_f32(nova_vec4f a, nova_vec4f b,
                                               nova_vec4f c) {
  return vfmaq_f32(c, a, b);
}
static inline nova_vec4f nova_simd_load_f32(const float *ptr) {
  return vld1q_f32(ptr);
}
static inline void nova_simd_store_f32(float *ptr, nova_vec4f v) {
  vst1q_f32(ptr, v);
}
static inline float nova_simd_reduce_sum_f32(nova_vec4f v) {
  return vaddvq_f32(v);
}
#endif

// Matrix operations (SIMD-optimized)
void nova_simd_matmul_f32(const float *A, const float *B, float *C, size_t M,
                            size_t N, size_t K);
void nova_simd_vec_add_f32(const float *a, const float *b, float *c,
                             size_t n);
void nova_simd_vec_mul_f32(const float *a, const float *b, float *c,
                             size_t n);
float nova_simd_dot_product_f32(const float *a, const float *b, size_t n);

// Prefetching hints
#define NOVA_PREFETCH_READ(addr) __builtin_prefetch(addr, 0, 3)
#define NOVA_PREFETCH_WRITE(addr) __builtin_prefetch(addr, 1, 3)
#define NOVA_PREFETCH_TEMPORAL(addr) __builtin_prefetch(addr, 0, 3)
#define NOVA_PREFETCH_NON_TEMPORAL(addr) __builtin_prefetch(addr, 0, 0)

// Cache control
#define NOVA_CACHE_LINE_SIZE 64
#define NOVA_ALIGN_CACHE __attribute__((aligned(NOVA_CACHE_LINE_SIZE)))

#endif // NOVA_SIMD_H