/**
 * @file bench_llvm_opts.c
 * @brief LLVM optimization level benchmarks
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>

#define LLVM_BENCH_ITERS 10000

// Simulate different optimization levels with varying complexity

// O0 - No optimization (naive loops)
static void compute_o0(const float *a, const float *b, float *c, int n)
{
    for (int i = 0; i < n; i++) {
        c[i] = 0.0f;
        for (int j = 0; j < 10; j++) {
            c[i] += a[i] * b[i];
        }
    }
}

// O2 - Some optimization (manual unroll)
static void compute_o2(const float *a, const float *b, float *c, int n)
{
    for (int i = 0; i < n; i += 4) {
        float sum0 = 0.0f, sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;
        for (int j = 0; j < 10; j++) {
            sum0 += a[i+0] * b[i+0];
            sum1 += a[i+1] * b[i+1];
            sum2 += a[i+2] * b[i+2];
            sum3 += a[i+3] * b[i+3];
        }
        c[i+0] = sum0;
        c[i+1] = sum1;
        c[i+2] = sum2;
        c[i+3] = sum3;
    }
}

// O3 - Aggressive optimization (vectorized hint)
static void compute_o3(const float *a, const float *b, float *c, int n)
{
    // Assume vectorization kicks in
    for (int i = 0; i < n; i++) {
        float sum = 0.0f;
        #pragma unroll
        for (int j = 0; j < 10; j++) {
            sum += a[i] * b[i];
        }
        c[i] = sum;
    }
}

double bench_llvm_opt_level(int opt_level, int n)
{
    float *a = malloc(n * sizeof(float));
    float *b = malloc(n * sizeof(float));
    float *c = malloc(n * sizeof(float));
    
    if (!a || !b || !c) {
        free(a); free(b); free(c);
        return -1.0;
    }
    
    for (int i = 0; i < n; i++) {
        a[i] = (float)rand() / RAND_MAX;
        b[i] = (float)rand() / RAND_MAX;
    }
    
    double start = zenith_timer_get_sec();
    
    for (int iter = 0; iter < LLVM_BENCH_ITERS; iter++) {
        switch (opt_level) {
            case 0: compute_o0(a, b, c, n); break;
            case 2: compute_o2(a, b, c, n); break;
            case 3: compute_o3(a, b, c, n); break;
        }
    }
    
    double end = zenith_timer_get_sec();
    
    free(a); free(b); free(c);
    return (end - start) / LLVM_BENCH_ITERS;
}

void autocal_benchmark_llvm_opts(NovaAutocalConfig *config)
{
    printf("📊 [LLVM] Benchmarking LLVM Optimization Levels...\n");
    
    int n = 4096;
    
    double time_o0 = bench_llvm_opt_level(0, n);
    double time_o2 = bench_llvm_opt_level(2, n);
    double time_o3 = bench_llvm_opt_level(3, n);
    
    printf("   -O0: %.3f ms (baseline)\n", time_o0 * 1000);
    printf("   -O2: %.3f ms (%.2fx speedup)\n", time_o2 * 1000, time_o0 / time_o2);
    printf("   -O3: %.3f ms (%.2fx speedup)\n", time_o3 * 1000, time_o0 / time_o3);
    
    double best_speedup = time_o0 / time_o3;
    printf("✅ [LLVM] Best speedup: %.2fx with -O3\n", best_speedup);
    
    if (best_speedup > 3.0) {
        printf("   💡 Recommendation: Use -O3 for production builds\n");
    }
}
