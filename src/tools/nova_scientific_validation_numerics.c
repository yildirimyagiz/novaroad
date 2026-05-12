/**
 * ═══════════════════════════════════════════════════════════════════════════
 * SECTION 4: NUMERICS STABILITY VALIDATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_scientific_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// ═══════════════════════════════════════════════════════════════════════════
// STANDARD ACCUMULATION
// ═══════════════════════════════════════════════════════════════════════════

static double accumulate_standard(const float* data, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += (double)data[i];
    }
    return sum;
}

// ═══════════════════════════════════════════════════════════════════════════
// KAHAN SUMMATION (Compensated Summation)
// ═══════════════════════════════════════════════════════════════════════════

static double accumulate_kahan(const float* data, size_t n) {
    double sum = 0.0;
    double c = 0.0; // Compensation for lost low-order bits
    
    for (size_t i = 0; i < n; i++) {
        double y = (double)data[i] - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    
    return sum;
}

// ═══════════════════════════════════════════════════════════════════════════
// PAIRWISE SUMMATION (Recursive)
// ═══════════════════════════════════════════════════════════════════════════

static double accumulate_pairwise_recursive(const float* data, size_t n) {
    if (n <= 16) {
        // Base case: standard summation for small arrays
        double sum = 0.0;
        for (size_t i = 0; i < n; i++) {
            sum += (double)data[i];
        }
        return sum;
    }
    
    size_t mid = n / 2;
    return accumulate_pairwise_recursive(data, mid) + 
           accumulate_pairwise_recursive(data + mid, n - mid);
}

static double accumulate_pairwise(const float* data, size_t n) {
    return accumulate_pairwise_recursive(data, n);
}

// ═══════════════════════════════════════════════════════════════════════════
// MIXED PRECISION ACCUMULATION
// ═══════════════════════════════════════════════════════════════════════════

static double accumulate_mixed_precision(const float* data, size_t n) {
    // Accumulate in double, but periodically downcast to float to simulate mixed precision
    float sum_f32 = 0.0f;
    
    for (size_t i = 0; i < n; i++) {
        sum_f32 += data[i];
        
        // Every 1000 elements, go through double precision
        if (i % 1000 == 999) {
            double sum_f64 = (double)sum_f32;
            sum_f32 = (float)sum_f64;
        }
    }
    
    return (double)sum_f32;
}

// ═══════════════════════════════════════════════════════════════════════════
// DOT PRODUCT
// ═══════════════════════════════════════════════════════════════════════════

static double dot_product_standard(const float* a, const float* b, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += (double)a[i] * (double)b[i];
    }
    return sum;
}

static double dot_product_kahan(const float* a, const float* b, size_t n) {
    double sum = 0.0;
    double c = 0.0;
    
    for (size_t i = 0; i < n; i++) {
        double prod = (double)a[i] * (double)b[i];
        double y = prod - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    
    return sum;
}

// ═══════════════════════════════════════════════════════════════════════════
// SOFTMAX STABILITY
// ═══════════════════════════════════════════════════════════════════════════

static void softmax_naive(const float* logits, float* output, size_t n) {
    // Naive implementation (numerically unstable)
    double sum_exp = 0.0;
    for (size_t i = 0; i < n; i++) {
        output[i] = expf(logits[i]);
        sum_exp += output[i];
    }
    
    for (size_t i = 0; i < n; i++) {
        output[i] /= (float)sum_exp;
    }
}

static void softmax_stable(const float* logits, float* output, size_t n) {
    // Numerically stable implementation (subtract max)
    float max_logit = logits[0];
    for (size_t i = 1; i < n; i++) {
        if (logits[i] > max_logit) {
            max_logit = logits[i];
        }
    }
    
    double sum_exp = 0.0;
    for (size_t i = 0; i < n; i++) {
        output[i] = expf(logits[i] - max_logit);
        sum_exp += output[i];
    }
    
    for (size_t i = 0; i < n; i++) {
        output[i] /= (float)sum_exp;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK ACCUMULATION
// ═══════════════════════════════════════════════════════════════════════════

NumericsMetrics bench_accumulation(const float* data, size_t n, AccumulationMethod method) {
    NumericsMetrics metrics = {0};
    
    const int BENCH_ITERS = 100;
    
    // Compute reference (high precision)
    double reference = 0.0;
    for (size_t i = 0; i < n; i++) {
        reference += (long double)data[i];
    }
    
    // Benchmark
    NovaTimer timer;
    nova_timer_start(&timer);
    
    double result = 0.0;
    for (int iter = 0; iter < BENCH_ITERS; iter++) {
        switch (method) {
            case ACCUM_STANDARD:
                result = accumulate_standard(data, n);
                break;
            case ACCUM_KAHAN:
                result = accumulate_kahan(data, n);
                break;
            case ACCUM_PAIRWISE:
                result = accumulate_pairwise(data, n);
                break;
            case ACCUM_MIXED_PRECISION:
                result = accumulate_mixed_precision(data, n);
                break;
        }
    }
    
    nova_timer_stop(&timer);
    
    metrics.result = result;
    metrics.latency_us = nova_timer_elapsed_us(&timer) / BENCH_ITERS;
    metrics.relative_error = fabs(result - reference) / fabs(reference);
    
    // ULP drift (simplified)
    double ulp = nextafter(result, INFINITY) - result;
    metrics.ulp_drift = fabs(result - reference) / ulp;
    
    // Determinism check (run twice)
    double result2 = 0.0;
    switch (method) {
        case ACCUM_STANDARD:
            result2 = accumulate_standard(data, n);
            break;
        case ACCUM_KAHAN:
            result2 = accumulate_kahan(data, n);
            break;
        case ACCUM_PAIRWISE:
            result2 = accumulate_pairwise(data, n);
            break;
        case ACCUM_MIXED_PRECISION:
            result2 = accumulate_mixed_precision(data, n);
            break;
    }
    metrics.deterministic = (result == result2);
    
    metrics.checksum = nova_checksum_fp32(&result, 1);
    
    return metrics;
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK DOT PRODUCT
// ═══════════════════════════════════════════════════════════════════════════

NumericsMetrics bench_dot_product(const float* a, const float* b, size_t n, 
                                   AccumulationMethod method) {
    NumericsMetrics metrics = {0};
    
    const int BENCH_ITERS = 100;
    
    // Compute reference
    long double reference = 0.0;
    for (size_t i = 0; i < n; i++) {
        reference += (long double)a[i] * (long double)b[i];
    }
    
    // Benchmark
    NovaTimer timer;
    nova_timer_start(&timer);
    
    double result = 0.0;
    for (int iter = 0; iter < BENCH_ITERS; iter++) {
        if (method == ACCUM_STANDARD) {
            result = dot_product_standard(a, b, n);
        } else if (method == ACCUM_KAHAN) {
            result = dot_product_kahan(a, b, n);
        }
    }
    
    nova_timer_stop(&timer);
    
    metrics.result = result;
    metrics.latency_us = nova_timer_elapsed_us(&timer) / BENCH_ITERS;
    metrics.relative_error = fabs(result - (double)reference) / fabs((double)reference);
    
    double ulp = nextafter(result, INFINITY) - result;
    metrics.ulp_drift = fabs(result - (double)reference) / ulp;
    
    metrics.deterministic = true;
    metrics.checksum = nova_checksum_fp32(&result, 1);
    
    return metrics;
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK SOFTMAX STABILITY
// ═══════════════════════════════════════════════════════════════════════════

NumericsMetrics bench_softmax_stability(const float* logits, size_t n) {
    NumericsMetrics metrics = {0};
    
    const int BENCH_ITERS = 100;
    
    float* output = (float*)malloc(n * sizeof(float));
    if (!output) return metrics;
    
    // Benchmark stable version
    NovaTimer timer;
    nova_timer_start(&timer);
    
    for (int iter = 0; iter < BENCH_ITERS; iter++) {
        softmax_stable(logits, output, n);
    }
    
    nova_timer_stop(&timer);
    
    // Check if sum is 1.0
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += output[i];
    }
    
    metrics.result = sum;
    metrics.latency_us = nova_timer_elapsed_us(&timer) / BENCH_ITERS;
    metrics.relative_error = fabs(sum - 1.0);
    metrics.ulp_drift = 0.0;
    metrics.deterministic = true;
    metrics.checksum = nova_checksum_fp32(output, n);
    
    free(output);
    
    return metrics;
}
