/**
 * @file bench_quantization.c
 * @brief Quantization benchmark (FP32 vs INT8)
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define QUANT_BENCH_ITERS 5000

// FP32 matmul
static void matmul_fp32(const float *a, const float *b, float *c, int m, int n, int k)
{
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int p = 0; p < k; p++) {
                sum += a[i * k + p] * b[p * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

// INT8 matmul (simulated)
static void matmul_int8(const int8_t *a, const int8_t *b, int32_t *c, int m, int n, int k)
{
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            int32_t sum = 0;
            for (int p = 0; p < k; p++) {
                sum += (int32_t)a[i * k + p] * (int32_t)b[p * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

// Quantize FP32 -> INT8
static void quantize_fp32_to_int8(const float *input, int8_t *output, int n, float scale)
{
    for (int i = 0; i < n; i++) {
        float scaled = input[i] / scale;
        if (scaled > 127.0f) scaled = 127.0f;
        if (scaled < -128.0f) scaled = -128.0f;
        output[i] = (int8_t)scaled;
    }
}

double bench_quantization_speedup(int m, int n, int k)
{
    // FP32 data
    float *a_fp32 = malloc(m * k * sizeof(float));
    float *b_fp32 = malloc(k * n * sizeof(float));
    float *c_fp32 = malloc(m * n * sizeof(float));
    
    // INT8 data
    int8_t *a_int8 = malloc(m * k * sizeof(int8_t));
    int8_t *b_int8 = malloc(k * n * sizeof(int8_t));
    int32_t *c_int8 = malloc(m * n * sizeof(int32_t));
    
    if (!a_fp32 || !b_fp32 || !c_fp32 || !a_int8 || !b_int8 || !c_int8) {
        free(a_fp32); free(b_fp32); free(c_fp32);
        free(a_int8); free(b_int8); free(c_int8);
        return -1.0;
    }
    
    // Initialize FP32
    for (int i = 0; i < m * k; i++) a_fp32[i] = (float)rand() / RAND_MAX;
    for (int i = 0; i < k * n; i++) b_fp32[i] = (float)rand() / RAND_MAX;
    
    // Quantize
    quantize_fp32_to_int8(a_fp32, a_int8, m * k, 1.0f / 127.0f);
    quantize_fp32_to_int8(b_fp32, b_int8, k * n, 1.0f / 127.0f);
    
    // Benchmark FP32
    double start = zenith_timer_get_sec();
    for (int i = 0; i < QUANT_BENCH_ITERS; i++) {
        matmul_fp32(a_fp32, b_fp32, c_fp32, m, n, k);
    }
    double time_fp32 = zenith_timer_get_sec() - start;
    
    // Benchmark INT8
    start = zenith_timer_get_sec();
    for (int i = 0; i < QUANT_BENCH_ITERS; i++) {
        matmul_int8(a_int8, b_int8, c_int8, m, n, k);
    }
    double time_int8 = zenith_timer_get_sec() - start;
    
    free(a_fp32); free(b_fp32); free(c_fp32);
    free(a_int8); free(b_int8); free(c_int8);
    
    return time_fp32 / time_int8; // speedup
}

void autocal_benchmark_quantization(NovaAutocalConfig *config)
{
    printf("📊 [QUANT] Benchmarking Quantization (FP32 vs INT8)...\n");
    
    struct {
        int m, n, k;
        const char *name;
    } sizes[] = {
        {32, 32, 32, "Small (32x32)"},
        {64, 64, 64, "Medium (64x64)"},
        {128, 128, 128, "Large (128x128)"}
    };
    
    double total_speedup = 0.0;
    for (int i = 0; i < 3; i++) {
        double speedup = bench_quantization_speedup(sizes[i].m, sizes[i].n, sizes[i].k);
        printf("   %s: %.2fx speedup\n", sizes[i].name, speedup);
        total_speedup += speedup;
    }
    
    double avg_speedup = total_speedup / 3.0;
    printf("✅ [QUANT] Average INT8 speedup: %.2fx\n", avg_speedup);
    
    if (avg_speedup > 2.0) {
        printf("   💡 Recommendation: Enable INT8 quantization for inference\n");
    }
}
