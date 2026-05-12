/**
 * @file simd_ops.c
 * @brief SIMD-optimized tensor operations (AVX2/NEON)
 */

#include "ai/tensor.h"
#include "platform/simd.h"
#include <string.h>
#include <math.h>

/* SIMD vector add - platform specific */
void nova_tensor_add_simd(const float *a, const float *b, float *out, size_t n)
{
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&out[i], vc);
    }
    /* Scalar remainder */
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t vc = vaddq_f32(va, vb);
        vst1q_f32(&out[i], vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
#else
    /* Fallback: scalar */
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i];
    }
#endif
}

/* SIMD vector multiply */
void nova_tensor_mul_simd(const float *a, const float *b, float *out, size_t n)
{
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vc = _mm256_mul_ps(va, vb);
        _mm256_storeu_ps(&out[i], vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] * b[i];
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t vc = vmulq_f32(va, vb);
        vst1q_f32(&out[i], vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] * b[i];
    }
#else
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] * b[i];
    }
#endif
}

/* SIMD dot product */
float nova_tensor_dot_simd(const float *a, const float *b, size_t n)
{
    float sum = 0.0f;
#if defined(__AVX2__)
    __m256 vsum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        vsum = _mm256_fmadd_ps(va, vb, vsum);
    }
    /* Horizontal sum */
    float temp[8];
    _mm256_storeu_ps(temp, vsum);
    for (int j = 0; j < 8; j++) sum += temp[j];
    
    /* Remainder */
    for (; i < n; i++) {
        sum += a[i] * b[i];
    }
#elif defined(__ARM_NEON)
    float32x4_t vsum = vdupq_n_f32(0.0f);
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        vsum = vmlaq_f32(vsum, va, vb);
    }
    /* Horizontal sum */
    sum = vgetq_lane_f32(vsum, 0) + vgetq_lane_f32(vsum, 1) +
          vgetq_lane_f32(vsum, 2) + vgetq_lane_f32(vsum, 3);
    
    for (; i < n; i++) {
        sum += a[i] * b[i];
    }
#else
    for (size_t i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
#endif
    return sum;
}

/* SIMD ReLU activation */
void nova_tensor_relu_simd(const float *in, float *out, size_t n)
{
#if defined(__AVX2__)
    __m256 vzero = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 vin = _mm256_loadu_ps(&in[i]);
        __m256 vout = _mm256_max_ps(vin, vzero);
        _mm256_storeu_ps(&out[i], vout);
    }
    for (; i < n; i++) {
        out[i] = (in[i] > 0.0f) ? in[i] : 0.0f;
    }
#elif defined(__ARM_NEON)
    float32x4_t vzero = vdupq_n_f32(0.0f);
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t vin = vld1q_f32(&in[i]);
        float32x4_t vout = vmaxq_f32(vin, vzero);
        vst1q_f32(&out[i], vout);
    }
    for (; i < n; i++) {
        out[i] = (in[i] > 0.0f) ? in[i] : 0.0f;
    }
#else
    for (size_t i = 0; i < n; i++) {
        out[i] = (in[i] > 0.0f) ? in[i] : 0.0f;
    }
#endif
}
