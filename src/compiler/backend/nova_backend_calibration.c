/**
 * @file nova_backend_calibration.c
 * @brief Auto-calibration system for backend selection
 * 
 * Runs micro-benchmarks at startup to determine optimal backend
 * for different workload sizes and operation types.
 */

#include "nova_backend_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CALIBRATION_ITERATIONS 10
#define WARMUP_ITERATIONS 3

typedef struct {
    int64_t size;
    NovaBackendType fastest_backend;
    uint64_t min_latency_ns;
} calibration_point_t;

typedef struct {
    calibration_point_t small;   // < 512
    calibration_point_t medium;  // 512-2048
    calibration_point_t large;   // > 2048
} matmul_calibration_t;

static matmul_calibration_t g_matmul_cal = {0};
static int g_calibration_done = 0;

// Helper to allocate and fill test matrices
static float* create_test_matrix(int64_t rows, int64_t cols)
{
    float *mat = (float*)malloc(rows * cols * sizeof(float));
    if (!mat) return NULL;
    
    for (int64_t i = 0; i < rows * cols; i++) {
        mat[i] = (float)(rand() % 100) / 100.0f;
    }
    return mat;
}

// Benchmark a single backend for matmul
static uint64_t benchmark_backend_matmul(NovaBackendType backend, 
                                          int64_t m, int64_t n, int64_t k)
{
    float *a = create_test_matrix(m, k);
    float *b = create_test_matrix(k, n);
    float *c = create_test_matrix(m, n);
    
    if (!a || !b || !c) {
        free(a); free(b); free(c);
        return UINT64_MAX;
    }
    
    // Save current backend
    NovaBackendStatus status = nova_backend_status();
    NovaBackendType original = status.active;
    
    // Temporarily switch backend (hack - direct access)
    extern NovaBackendStatus g_status;
    g_status.active = backend;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        nova_dispatch_matmul(a, b, c, m, n, k);
    }
    
    // Actual benchmark
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < CALIBRATION_ITERATIONS; i++) {
        int64_t ret = nova_dispatch_matmul(a, b, c, m, n, k);
        if (ret < 0) {
            // Backend failed
            free(a); free(b); free(c);
            g_status.active = original;
            return UINT64_MAX;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Restore original backend
    g_status.active = original;
    
    uint64_t total_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL +
                        (end.tv_nsec - start.tv_nsec);
    uint64_t avg_ns = total_ns / CALIBRATION_ITERATIONS;
    
    free(a); free(b); free(c);
    return avg_ns;
}

// Calibrate for a specific size
static calibration_point_t calibrate_matmul_size(int64_t size)
{
    calibration_point_t result = {0};
    result.size = size;
    result.min_latency_ns = UINT64_MAX;
    result.fastest_backend = NOVA_BACKEND_CPU;
    
    printf("  📊 Calibrating matmul [%ldx%ld]... ", size, size);
    fflush(stdout);
    
    NovaBackendStatus status = nova_backend_status();
    NovaBackendType backends[] = {
        NOVA_BACKEND_CPU,
        NOVA_BACKEND_CUDA,
        NOVA_BACKEND_METAL,
        NOVA_BACKEND_ROCM,
        NOVA_BACKEND_VULKAN,
        NOVA_BACKEND_OPENCL,
        NOVA_BACKEND_GPU_ARMY
    };
    
    bool available[] = {
        true,
        status.cuda_available,
        status.metal_available,
        status.rocm_available,
        status.vulkan_available,
        status.opencl_available,
        status.cuda_available  // GPU_ARMY requires CUDA
    };
    
    for (int i = 0; i < 7; i++) {
        if (!available[i]) continue;
        
        uint64_t latency = benchmark_backend_matmul(backends[i], size, size, size);
        
        if (latency < result.min_latency_ns) {
            result.min_latency_ns = latency;
            result.fastest_backend = backends[i];
        }
    }
    
    printf("Best: %s (%lu μs)\n", 
           nova_backend_name(result.fastest_backend),
           result.min_latency_ns / 1000);
    
    return result;
}

// Main calibration routine
void nova_backend_calibrate(void)
{
    if (g_calibration_done) {
        printf("⚠️  Calibration already done, skipping...\n");
        return;
    }
    
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║         Nova Backend Auto-Calibration Starting...            ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ Testing different workload sizes to find optimal backends... ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    // Calibrate different sizes
    g_matmul_cal.small = calibrate_matmul_size(256);
    g_matmul_cal.medium = calibrate_matmul_size(1024);
    g_matmul_cal.large = calibrate_matmul_size(4096);
    
    g_calibration_done = 1;
    
    printf("\n✅ Calibration complete!\n");
    printf("   Small  (256):  %s\n", nova_backend_name(g_matmul_cal.small.fastest_backend));
    printf("   Medium (1024): %s\n", nova_backend_name(g_matmul_cal.medium.fastest_backend));
    printf("   Large  (4096): %s\n", nova_backend_name(g_matmul_cal.large.fastest_backend));
    printf("\n");
}

// Get recommended backend based on calibration
NovaBackendType nova_backend_recommend_for_matmul(int64_t m, int64_t n, int64_t k)
{
    if (!g_calibration_done) {
        return NOVA_BACKEND_AUTO;  // Not calibrated yet
    }
    
    int64_t max_dim = m > n ? m : n;
    max_dim = max_dim > k ? max_dim : k;
    
    if (max_dim < 512) {
        return g_matmul_cal.small.fastest_backend;
    } else if (max_dim < 2048) {
        return g_matmul_cal.medium.fastest_backend;
    } else {
        return g_matmul_cal.large.fastest_backend;
    }
}

int nova_backend_is_calibrated(void)
{
    return g_calibration_done;
}

void nova_backend_save_calibration(const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("❌ Failed to save calibration to %s\n", filename);
        return;
    }
    
    fprintf(f, "# Nova Backend Calibration Data\n");
    fprintf(f, "small_backend=%d\n", g_matmul_cal.small.fastest_backend);
    fprintf(f, "small_latency=%lu\n", g_matmul_cal.small.min_latency_ns);
    fprintf(f, "medium_backend=%d\n", g_matmul_cal.medium.fastest_backend);
    fprintf(f, "medium_latency=%lu\n", g_matmul_cal.medium.min_latency_ns);
    fprintf(f, "large_backend=%d\n", g_matmul_cal.large.fastest_backend);
    fprintf(f, "large_latency=%lu\n", g_matmul_cal.large.min_latency_ns);
    
    fclose(f);
    printf("💾 Calibration saved to %s\n", filename);
}

void nova_backend_load_calibration(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("⚠️  No calibration file found at %s\n", filename);
        return;
    }
    
    int small_b, medium_b, large_b;
    uint64_t small_l, medium_l, large_l;
    
    if (fscanf(f, "# Nova Backend Calibration Data\n") < 0 ||
        fscanf(f, "small_backend=%d\n", &small_b) != 1 ||
        fscanf(f, "small_latency=%lu\n", &small_l) != 1 ||
        fscanf(f, "medium_backend=%d\n", &medium_b) != 1 ||
        fscanf(f, "medium_latency=%lu\n", &medium_l) != 1 ||
        fscanf(f, "large_backend=%d\n", &large_b) != 1 ||
        fscanf(f, "large_latency=%lu\n", &large_l) != 1) {
        printf("❌ Failed to parse calibration file\n");
        fclose(f);
        return;
    }
    
    g_matmul_cal.small.fastest_backend = small_b;
    g_matmul_cal.small.min_latency_ns = small_l;
    g_matmul_cal.medium.fastest_backend = medium_b;
    g_matmul_cal.medium.min_latency_ns = medium_l;
    g_matmul_cal.large.fastest_backend = large_b;
    g_matmul_cal.large.min_latency_ns = large_l;
    
    g_calibration_done = 1;
    
    fclose(f);
    printf("📂 Calibration loaded from %s\n", filename);
}
