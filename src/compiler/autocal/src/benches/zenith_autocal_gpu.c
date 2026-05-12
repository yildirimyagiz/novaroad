/**
 * @file zenith_autocal_gpu.c
 * @brief GPU benchmark support (Metal/CUDA)
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for backend GPU functions
#ifdef __APPLE__
extern int64_t nova_metal_init(void);
extern int64_t nova_metal_matmul(const float *a, const float *b, float *c, 
                                  int64_t m, int64_t n, int64_t k);
extern void nova_metal_cleanup(void);
#endif

extern int64_t nova_cuda_init(void);
extern int64_t nova_cuda_matmul(const float *a, const float *b, float *c,
                                 int64_t m, int64_t n, int64_t k);
extern void nova_cuda_cleanup(void);

#define GPU_WARMUP_ITERS 3
#define GPU_BENCH_ITERS 10

/**
 * Benchmark Metal GPU performance (macOS only)
 */
double bench_metal_matmul(int m, int n, int k)
{
#ifdef __APPLE__
    if (nova_metal_init() != 1) {
        printf("   ⚠️  Metal not available\n");
        return -1.0;
    }
    
    float *a = malloc(m * k * sizeof(float));
    float *b = malloc(k * n * sizeof(float));
    float *c = malloc(m * n * sizeof(float));
    
    if (!a || !b || !c) {
        free(a); free(b); free(c);
        return -1.0;
    }
    
    // Initialize
    for (int i = 0; i < m * k; i++) a[i] = (float)rand() / RAND_MAX;
    for (int i = 0; i < k * n; i++) b[i] = (float)rand() / RAND_MAX;
    
    // Warmup
    for (int i = 0; i < GPU_WARMUP_ITERS; i++) {
        nova_metal_matmul(a, b, c, m, n, k);
    }
    
    // Benchmark
    double start = zenith_timer_get_sec();
    for (int i = 0; i < GPU_BENCH_ITERS; i++) {
        nova_metal_matmul(a, b, c, m, n, k);
    }
    double end = zenith_timer_get_sec();
    
    free(a); free(b); free(c);
    nova_metal_cleanup();
    
    return (end - start) / GPU_BENCH_ITERS;
#else
    (void)m; (void)n; (void)k;
    return -1.0;
#endif
}

/**
 * Benchmark CUDA GPU performance
 */
double bench_cuda_matmul(int m, int n, int k)
{
    if (nova_cuda_init() != 1) {
        printf("   ⚠️  CUDA not available\n");
        return -1.0;
    }
    
    float *a = malloc(m * k * sizeof(float));
    float *b = malloc(k * n * sizeof(float));
    float *c = malloc(m * n * sizeof(float));
    
    if (!a || !b || !c) {
        free(a); free(b); free(c);
        return -1.0;
    }
    
    // Initialize
    for (int i = 0; i < m * k; i++) a[i] = (float)rand() / RAND_MAX;
    for (int i = 0; i < k * n; i++) b[i] = (float)rand() / RAND_MAX;
    
    // Warmup
    for (int i = 0; i < GPU_WARMUP_ITERS; i++) {
        nova_cuda_matmul(a, b, c, m, n, k);
    }
    
    // Benchmark
    double start = zenith_timer_get_sec();
    for (int i = 0; i < GPU_BENCH_ITERS; i++) {
        nova_cuda_matmul(a, b, c, m, n, k);
    }
    double end = zenith_timer_get_sec();
    
    free(a); free(b); free(c);
    nova_cuda_cleanup();
    
    return (end - start) / GPU_BENCH_ITERS;
}

/**
 * Calculate GFLOPS for matmul
 */
static double calculate_gflops(int m, int n, int k, double time_sec)
{
    if (time_sec <= 0) return 0.0;
    double flops = 2.0 * m * n * k; // 2 ops per multiply-add
    return flops / time_sec / 1e9;
}

void autocal_benchmark_gpu_performance(NovaAutocalConfig *config)
{
    printf("📊 [GPU] Benchmarking GPU Performance...\n");
    
    int m = 1024, n = 1024, k = 1024;
    
    // Try Metal
#ifdef __APPLE__
    printf("   Testing Metal GPU...\n");
    double metal_time = bench_metal_matmul(m, n, k);
    if (metal_time > 0) {
        double metal_gflops = calculate_gflops(m, n, k, metal_time);
        printf("   ✅ Metal: %.2f GFLOPS (%.3f ms)\n", metal_gflops, metal_time * 1000);
        config->gpu_gflops = metal_gflops;
    }
#endif
    
    // Try CUDA
    printf("   Testing CUDA GPU...\n");
    double cuda_time = bench_cuda_matmul(m, n, k);
    if (cuda_time > 0) {
        double cuda_gflops = calculate_gflops(m, n, k, cuda_time);
        printf("   ✅ CUDA: %.2f GFLOPS (%.3f ms)\n", cuda_gflops, cuda_time * 1000);
        if (cuda_gflops > config->gpu_gflops) {
            config->gpu_gflops = cuda_gflops;
        }
    }
    
    if (config->gpu_gflops > 0) {
        printf("✅ [GPU] Best GPU performance: %.2f GFLOPS\n", config->gpu_gflops);
        
        // Compare with CPU
        if (config->cpu_gflops > 0) {
            double ratio = config->gpu_gflops / config->cpu_gflops;
            printf("   💡 GPU is %.2fx faster than CPU\n", ratio);
            
            // Recommend thresholds based on ratio
            if (ratio > 10.0) {
                printf("   💡 Recommendation: Use GPU for sizes > 256x256\n");
            } else if (ratio > 3.0) {
                printf("   💡 Recommendation: Use GPU for sizes > 512x512\n");
            } else {
                printf("   💡 Recommendation: Use GPU for sizes > 1024x1024\n");
            }
        }
    } else {
        printf("⚠️  [GPU] No GPU available, using CPU-only mode\n");
        config->gpu_gflops = 0.0;
    }
}
