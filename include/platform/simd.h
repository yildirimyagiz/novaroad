/**
 * @file simd.h
 * @brief SIMD intrinsics abstraction (enhanced)
 */

#ifndef NOVA_SIMD_H
#define NOVA_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Platform detection */
#if defined(__x86_64__) || defined(_M_X64)
    #define NOVA_SIMD_AVX2 1
    #include <immintrin.h>
    typedef __m256 nova_simd_f32x8_t;
    typedef __m256d nova_simd_f64x4_t;
    typedef __m256i nova_simd_i32x8_t;
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define NOVA_SIMD_NEON 1
    #include <arm_neon.h>
    typedef float32x4_t nova_simd_f32x4_t;
    typedef float64x2_t nova_simd_f64x2_t;
    typedef int32x4_t nova_simd_i32x4_t;
#else
    #define NOVA_SIMD_NONE 1
#endif

/* ============================================================================
 * SIMD Availability
 * ========================================================================== */

bool nova_simd_available(void);
const char *nova_simd_name(void);

/* ============================================================================
 * Vector Operations (f32)
 * ========================================================================== */

void nova_simd_add_f32(const float *a, const float *b, float *result, size_t len);
void nova_simd_sub_f32(const float *a, const float *b, float *result, size_t len);
void nova_simd_mul_f32(const float *a, const float *b, float *result, size_t len);
void nova_simd_div_f32(const float *a, const float *b, float *result, size_t len);

/* ============================================================================
 * Reduction Operations
 * ========================================================================== */

float nova_simd_sum_f32(const float *array, size_t len);
float nova_simd_max_f32(const float *array, size_t len);
float nova_simd_min_f32(const float *array, size_t len);
float nova_simd_dot_f32(const float *a, const float *b, size_t len);

/* ============================================================================
 * Mathematical Functions
 * ========================================================================== */

void nova_simd_sqrt_f32(const float *input, float *output, size_t len);
void nova_simd_exp_f32(const float *input, float *output, size_t len);
void nova_simd_log_f32(const float *input, float *output, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_SIMD_H */
