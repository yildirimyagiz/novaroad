#include "compute/nova_kernels.h"
#include "ml/nova_tensor.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Platform-specific SIMD headers
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define NOVA_X86_SIMD 1
    #include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
    #define NOVA_ARM_NEON 1
    #include <arm_neon.h>
#endif

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA SIMD KERNELS - AVX2/AVX-512 Vectorization
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Feature detection
static int has_simd = -1;
static int simd_width = 0;

static void detect_cpu_features(void) {
    if (has_simd != -1) return;
    
#if defined(NOVA_X86_SIMD)
    #ifdef __GNUC__
        __builtin_cpu_init();
        has_simd = __builtin_cpu_supports("avx2") || __builtin_cpu_supports("sse4.2");
        simd_width = __builtin_cpu_supports("avx512f") ? 16 : 
                     (__builtin_cpu_supports("avx2") ? 8 : 4);
        printf("🚀 CPU: x86_64 SIMD (width: %d)\n", simd_width);
    #else
        has_simd = 0;
        simd_width = 0;
    #endif
#elif defined(NOVA_ARM_NEON)
    has_simd = 1;
    simd_width = 4;  // NEON processes 4 floats at a time
    printf("🚀 CPU: ARM NEON SIMD (width: 4)\n");
#else
    has_simd = 0;
    simd_width = 0;
    printf("🚀 CPU: Scalar (no SIMD)\n");
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// ARM NEON Kernels (4-wide float)
// ═══════════════════════════════════════════════════════════════════════════

#ifdef NOVA_ARM_NEON

void simd_kernel_relu_neon(const float *x, float *out, size_t n) {
    size_t i = 0;
    float32x4_t zero = vdupq_n_f32(0.0f);
    
    // Process 4 elements at a time
    for (; i + 3 < n; i += 4) {
        float32x4_t x_vec = vld1q_f32(&x[i]);
        float32x4_t result = vmaxq_f32(x_vec, zero);
        vst1q_f32(&out[i], result);
    }
    
    // Scalar tail
    for (; i < n; i++) {
        out[i] = x[i] > 0.0f ? x[i] : 0.0f;
    }
}

void simd_kernel_add_neon(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    
    for (; i + 3 < n; i += 4) {
        float32x4_t a_vec = vld1q_f32(&a[i]);
        float32x4_t b_vec = vld1q_f32(&b[i]);
        float32x4_t result = vaddq_f32(a_vec, b_vec);
        vst1q_f32(&c[i], result);
    }
    
    for (; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

void simd_kernel_mul_neon(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    
    for (; i + 3 < n; i += 4) {
        float32x4_t a_vec = vld1q_f32(&a[i]);
        float32x4_t b_vec = vld1q_f32(&b[i]);
        float32x4_t result = vmulq_f32(a_vec, b_vec);
        vst1q_f32(&c[i], result);
    }
    
    for (; i < n; i++) {
        c[i] = a[i] * b[i];
    }
}

#endif // NOVA_ARM_NEON

// ═══════════════════════════════════════════════════════════════════════════
// x86 AVX2 Kernels (8-wide float)
// ═══════════════════════════════════════════════════════════════════════════

#if defined(NOVA_X86_SIMD) && defined(__AVX2__)

void simd_kernel_relu_avx2(const float *x, float *out, size_t n) {
    size_t i = 0;
    __m256 zero = _mm256_setzero_ps();
    
    // Process 8 elements at a time
    for (; i + 7 < n; i += 8) {
        __m256 x_vec = _mm256_loadu_ps(&x[i]);
        __m256 result = _mm256_max_ps(x_vec, zero);
        _mm256_storeu_ps(&out[i], result);
    }
    
    // Scalar tail
    for (; i < n; i++) {
        out[i] = x[i] > 0.0f ? x[i] : 0.0f;
    }
}

void simd_kernel_gelu_avx2(const float *x, float *out, size_t n) {
    const float sqrt_2_over_pi = 0.7978845608f;
    const float coeff = 0.044715f;
    
    __m256 v_sqrt = _mm256_set1_ps(sqrt_2_over_pi);
    __m256 v_coeff = _mm256_set1_ps(coeff);
    __m256 v_half = _mm256_set1_ps(0.5f);
    __m256 v_one = _mm256_set1_ps(1.0f);
    
    size_t i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 x_vec = _mm256_loadu_ps(&x[i]);
        
        // x³
        __m256 x_sq = _mm256_mul_ps(x_vec, x_vec);
        __m256 x_cubed = _mm256_mul_ps(x_sq, x_vec);
        
        // inner = sqrt(2/π) * (x + 0.044715 * x³)
        __m256 term = _mm256_fmadd_ps(v_coeff, x_cubed, x_vec);
        __m256 inner = _mm256_mul_ps(v_sqrt, term);
        
        // tanh approximation: not available in AVX2, use scalar
        float inner_arr[8];
        _mm256_storeu_ps(inner_arr, inner);
        
        for (int j = 0; j < 8; j++) {
            float x_val = x[i + j];
            float tanh_val = tanhf(inner_arr[j]);
            out[i + j] = 0.5f * x_val * (1.0f + tanh_val);
        }
    }
    
    // Scalar tail
    for (; i < n; i++) {
        float x_val = x[i];
        float x_cubed = x_val * x_val * x_val;
        float inner = sqrt_2_over_pi * (x_val + coeff * x_cubed);
        out[i] = 0.5f * x_val * (1.0f + tanhf(inner));
    }
}

void simd_kernel_silu_avx2(const float *x, float *out, size_t n) {
    size_t i = 0;
    __m256 v_one = _mm256_set1_ps(1.0f);
    
    for (; i + 7 < n; i += 8) {
        __m256 x_vec = _mm256_loadu_ps(&x[i]);
        
        // Approximate sigmoid with exp (no native exp in AVX2)
        float x_arr[8];
        _mm256_storeu_ps(x_arr, x_vec);
        
        for (int j = 0; j < 8; j++) {
            float x_val = x[i + j];
            float sigmoid = 1.0f / (1.0f + expf(-x_val));
            out[i + j] = x_val * sigmoid;
        }
    }
    
    // Scalar tail
    for (; i < n; i++) {
        float x_val = x[i];
        float sigmoid = 1.0f / (1.0f + expf(-x_val));
        out[i] = x_val * sigmoid;
    }
}

void simd_kernel_add_avx2(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    
    for (; i + 7 < n; i += 8) {
        __m256 a_vec = _mm256_loadu_ps(&a[i]);
        __m256 b_vec = _mm256_loadu_ps(&b[i]);
        __m256 result = _mm256_add_ps(a_vec, b_vec);
        _mm256_storeu_ps(&c[i], result);
    }
    
    for (; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

void simd_kernel_mul_avx2(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    
    for (; i + 7 < n; i += 8) {
        __m256 a_vec = _mm256_loadu_ps(&a[i]);
        __m256 b_vec = _mm256_loadu_ps(&b[i]);
        __m256 result = _mm256_mul_ps(a_vec, b_vec);
        _mm256_storeu_ps(&c[i], result);
    }
    
    for (; i < n; i++) {
        c[i] = a[i] * b[i];
    }
}

#endif // NOVA_X86_SIMD && __AVX2__

// ═══════════════════════════════════════════════════════════════════════════
// x86 AVX-512 Kernels (16-wide float)
// ═══════════════════════════════════════════════════════════════════════════

#if defined(NOVA_X86_SIMD) && defined(__AVX512F__)

void simd_kernel_relu_avx512(const float *x, float *out, size_t n) {
    size_t i = 0;
    __m512 zero = _mm512_setzero_ps();
    
    // Process 16 elements at a time
    for (; i + 15 < n; i += 16) {
        __m512 x_vec = _mm512_loadu_ps(&x[i]);
        __m512 result = _mm512_max_ps(x_vec, zero);
        _mm512_storeu_ps(&out[i], result);
    }
    
    // Scalar tail
    for (; i < n; i++) {
        out[i] = x[i] > 0.0f ? x[i] : 0.0f;
    }
}

void simd_kernel_add_avx512(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    
    for (; i + 15 < n; i += 16) {
        __m512 a_vec = _mm512_loadu_ps(&a[i]);
        __m512 b_vec = _mm512_loadu_ps(&b[i]);
        __m512 result = _mm512_add_ps(a_vec, b_vec);
        _mm512_storeu_ps(&c[i], result);
    }
    
    for (; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

void simd_kernel_mul_avx512(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    
    for (; i + 15 < n; i += 16) {
        __m512 a_vec = _mm512_loadu_ps(&a[i]);
        __m512 b_vec = _mm512_loadu_ps(&b[i]);
        __m512 result = _mm512_mul_ps(a_vec, b_vec);
        _mm512_storeu_ps(&c[i], result);
    }
    
    for (; i < n; i++) {
        c[i] = a[i] * b[i];
    }
}

#endif // NOVA_X86_SIMD && __AVX512F__

// ═══════════════════════════════════════════════════════════════════════════
// High-level SIMD Dispatch
// ═══════════════════════════════════════════════════════════════════════════

void nova_simd_gelu(NovaTensor *x, NovaTensor *out) {
    detect_cpu_features();
    
    float *x_data = (float *)x->data;
    float *out_data = (float *)out->data;
    size_t n = x->total_elements;
    
#if defined(NOVA_X86_SIMD) && defined(__AVX2__)
    if (has_simd && simd_width >= 8 && n > 32) {
        simd_kernel_gelu_avx2(x_data, out_data, n);
        return;
    }
#endif
    
    // Fallback to scalar
    nova_kernel_gelu(x, out);
}

void nova_simd_silu(NovaTensor *x, NovaTensor *out) {
    detect_cpu_features();
    
    float *x_data = (float *)x->data;
    float *out_data = (float *)out->data;
    size_t n = x->total_elements;
    
#if defined(NOVA_X86_SIMD) && defined(__AVX2__)
    if (has_simd && simd_width >= 8 && n > 32) {
        simd_kernel_silu_avx2(x_data, out_data, n);
        return;
    }
#endif
    
    nova_kernel_silu(x, out);
}

void nova_simd_relu(NovaTensor *x, NovaTensor *out) {
    detect_cpu_features();
    
    float *x_data = (float *)x->data;
    float *out_data = (float *)out->data;
    size_t n = x->total_elements;
    
#if defined(NOVA_X86_SIMD) && defined(__AVX512F__)
    if (has_simd && simd_width >= 16 && n > 64) {
        simd_kernel_relu_avx512(x_data, out_data, n);
        return;
    }
#endif
    
#if defined(NOVA_X86_SIMD) && defined(__AVX2__)
    if (has_simd && simd_width >= 8 && n > 32) {
        simd_kernel_relu_avx2(x_data, out_data, n);
        return;
    }
#endif

#ifdef NOVA_ARM_NEON
    if (has_simd && n > 16) {
        simd_kernel_relu_neon(x_data, out_data, n);
        return;
    }
#endif
    
    nova_kernel_relu(x, out);
}

void nova_simd_add(NovaTensor *a, NovaTensor *b, NovaTensor *c) {
    detect_cpu_features();
    
    float *a_data = (float *)a->data;
    float *b_data = (float *)b->data;
    float *c_data = (float *)c->data;
    size_t n = a->total_elements;
    
#if defined(NOVA_X86_SIMD) && defined(__AVX512F__)
    if (has_simd && simd_width >= 16 && n > 64) {
        simd_kernel_add_avx512(a_data, b_data, c_data, n);
        return;
    }
#endif
    
#if defined(NOVA_X86_SIMD) && defined(__AVX2__)
    if (has_simd && simd_width >= 8 && n > 32) {
        simd_kernel_add_avx2(a_data, b_data, c_data, n);
        return;
    }
#endif

#ifdef NOVA_ARM_NEON
    if (has_simd && n > 16) {
        simd_kernel_add_neon(a_data, b_data, c_data, n);
        return;
    }
#endif
    
    nova_kernel_add(a, b, c);
}
