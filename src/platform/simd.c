/**
 * @file simd.c
 * @brief SIMD operations
 */

#include "platform/simd.h"
#include <stdbool.h>

bool nova_simd_available(void)
{
#if defined(NOVA_SIMD_AVX2) || defined(NOVA_SIMD_NEON)
    return true;
#else
    return false;
#endif
}

void nova_simd_add_f32(const float *a, const float *b, float *result, size_t len)
{
#if defined(NOVA_SIMD_AVX2)
    size_t i = 0;
    for (; i + 8 <= len; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vr = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&result[i], vr);
    }
    for (; i < len; i++) {
        result[i] = a[i] + b[i];
    }
#elif defined(NOVA_SIMD_NEON)
    size_t i = 0;
    for (; i + 4 <= len; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t vr = vaddq_f32(va, vb);
        vst1q_f32(&result[i], vr);
    }
    for (; i < len; i++) {
        result[i] = a[i] + b[i];
    }
#else
    for (size_t i = 0; i < len; i++) {
        result[i] = a[i] + b[i];
    }
#endif
}

void nova_simd_mul_f32(const float *a, const float *b, float *result, size_t len)
{
#if defined(NOVA_SIMD_AVX2)
    size_t i = 0;
    for (; i + 8 <= len; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vr = _mm256_mul_ps(va, vb);
        _mm256_storeu_ps(&result[i], vr);
    }
    for (; i < len; i++) {
        result[i] = a[i] * b[i];
    }
#elif defined(NOVA_SIMD_NEON)
    size_t i = 0;
    for (; i + 4 <= len; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t vr = vmulq_f32(va, vb);
        vst1q_f32(&result[i], vr);
    }
    for (; i < len; i++) {
        result[i] = a[i] * b[i];
    }
#else
    for (size_t i = 0; i < len; i++) {
        result[i] = a[i] * b[i];
    }
#endif
}
