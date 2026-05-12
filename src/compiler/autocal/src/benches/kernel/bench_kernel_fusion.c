/**
 * @file bench_kernel_fusion.c
 * @brief Kernel fusion optimization benchmarks
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define KERNEL_BENCH_ITERS 1000

// Non-fused: separate kernels
static void relu_kernel(const float *input, float *output, int n)
{
    for (int i = 0; i < n; i++) {
        output[i] = input[i] > 0.0f ? input[i] : 0.0f;
    }
}

static void add_kernel(const float *a, const float *b, float *output, int n)
{
    for (int i = 0; i < n; i++) {
        output[i] = a[i] + b[i];
    }
}

static void mul_kernel(const float *a, const float *b, float *output, int n)
{
    for (int i = 0; i < n; i++) {
        output[i] = a[i] * b[i];
    }
}

// Fused: relu(add(mul(a, b), c))
static void fused_kernel(const float *a, const float *b, const float *c, float *output, int n)
{
    for (int i = 0; i < n; i++) {
        float mul = a[i] * b[i];
        float add = mul + c[i];
        output[i] = add > 0.0f ? add : 0.0f;
    }
}

double bench_kernel_fusion_speedup(int n)
{
    float *a = malloc(n * sizeof(float));
    float *b = malloc(n * sizeof(float));
    float *c = malloc(n * sizeof(float));
    float *tmp1 = malloc(n * sizeof(float));
    float *tmp2 = malloc(n * sizeof(float));
    float *output = malloc(n * sizeof(float));
    
    if (!a || !b || !c || !tmp1 || !tmp2 || !output) {
        free(a); free(b); free(c); free(tmp1); free(tmp2); free(output);
        return -1.0;
    }
    
    for (int i = 0; i < n; i++) {
        a[i] = (float)rand() / RAND_MAX;
        b[i] = (float)rand() / RAND_MAX;
        c[i] = (float)rand() / RAND_MAX;
    }
    
    // Benchmark non-fused
    double start = zenith_timer_get_sec();
    for (int iter = 0; iter < KERNEL_BENCH_ITERS; iter++) {
        mul_kernel(a, b, tmp1, n);
        add_kernel(tmp1, c, tmp2, n);
        relu_kernel(tmp2, output, n);
    }
    double time_nonfused = zenith_timer_get_sec() - start;
    
    // Benchmark fused
    start = zenith_timer_get_sec();
    for (int iter = 0; iter < KERNEL_BENCH_ITERS; iter++) {
        fused_kernel(a, b, c, output, n);
    }
    double time_fused = zenith_timer_get_sec() - start;
    
    free(a); free(b); free(c); free(tmp1); free(tmp2); free(output);
    
    return time_nonfused / time_fused; // speedup ratio
}

void autocal_benchmark_kernel_fusion(NovaAutocalConfig *config)
{
    printf("📊 [KERNEL] Benchmarking Kernel Fusion...\n");
    
    int sizes[] = {1024, 4096, 16384, 65536};
    const char *names[] = {"1K", "4K", "16K", "64K"};
    
    double total_speedup = 0.0;
    for (int i = 0; i < 4; i++) {
        double speedup = bench_kernel_fusion_speedup(sizes[i]);
        printf("   Size %s: %.2fx speedup\n", names[i], speedup);
        total_speedup += speedup;
    }
    
    double avg_speedup = total_speedup / 4.0;
    printf("✅ [KERNEL] Average fusion speedup: %.2fx\n", avg_speedup);
    
    // Store hint: if fusion gives >2x, recommend it
    if (avg_speedup > 2.0) {
        printf("   💡 Recommendation: Enable kernel fusion (>2x speedup detected)\n");
    }
}
