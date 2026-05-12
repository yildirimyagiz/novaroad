/**
 * Matrix Multiplication Benchmark for Auto-Calibration
 * Tests various GEMM implementations and tile sizes
 */

#include "nova_autocal.h"
#include "nova_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t M, N, K;
    size_t tile_m, tile_n, tile_k;
} MatmulConfig;

typedef struct {
    double time_ms;
    double gflops;
    double efficiency_percent;
} MatmulResult;

// Naive matrix multiplication
static void matmul_naive(const float *A, const float *B, float *C, 
                        size_t M, size_t N, size_t K) {
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
            float sum = 0.0f;
            for (size_t k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// Tiled matrix multiplication
static void matmul_tiled(const float *A, const float *B, float *C,
                        const MatmulConfig *cfg) {
    size_t M = cfg->M, N = cfg->N, K = cfg->K;
    size_t TM = cfg->tile_m, TN = cfg->tile_n, TK = cfg->tile_k;
    
    for (size_t i = 0; i < M; i += TM) {
        for (size_t j = 0; j < N; j += TN) {
            for (size_t k = 0; k < K; k += TK) {
                // Tile boundaries
                size_t i_max = (i + TM < M) ? i + TM : M;
                size_t j_max = (j + TN < N) ? j + TN : N;
                size_t k_max = (k + TK < K) ? k + TK : K;
                
                // Tile computation
                for (size_t ii = i; ii < i_max; ii++) {
                    for (size_t jj = j; jj < j_max; jj++) {
                        float sum = (k == 0) ? 0.0f : C[ii * N + jj];
                        for (size_t kk = k; kk < k_max; kk++) {
                            sum += A[ii * K + kk] * B[kk * N + jj];
                        }
                        C[ii * N + jj] = sum;
                    }
                }
            }
        }
    }
}

static MatmulResult bench_matmul(const MatmulConfig *cfg, size_t iterations) {
    MatmulResult result = {0};
    
    // Allocate matrices
    float *A = (float*)malloc(cfg->M * cfg->K * sizeof(float));
    float *B = (float*)malloc(cfg->K * cfg->N * sizeof(float));
    float *C = (float*)malloc(cfg->M * cfg->N * sizeof(float));
    
    if (!A || !B || !C) {
        fprintf(stderr, "Failed to allocate memory for matmul\n");
        free(A); free(B); free(C);
        return result;
    }
    
    // Initialize
    for (size_t i = 0; i < cfg->M * cfg->K; i++) A[i] = (float)rand() / RAND_MAX;
    for (size_t i = 0; i < cfg->K * cfg->N; i++) B[i] = (float)rand() / RAND_MAX;
    
    // Warm-up
    matmul_tiled(A, B, C, cfg);
    
    // Benchmark
    nova_timer_t *timer = nova_timer_create();
    nova_timer_start(timer);
    
    for (size_t i = 0; i < iterations; i++) {
        matmul_tiled(A, B, C, cfg);
    }
    
    nova_timer_stop(timer);
    result.time_ms = nova_timer_elapsed_ms(timer) / iterations;
    
    // Calculate GFLOPS: 2*M*N*K operations
    double flops = 2.0 * cfg->M * cfg->N * cfg->K;
    result.gflops = (flops / 1e9) / (result.time_ms / 1000.0);
    
    // Theoretical peak (estimate)
    double peak_gflops = 100.0; // Adjust based on hardware
    result.efficiency_percent = (result.gflops / peak_gflops) * 100.0;
    
    nova_timer_destroy(timer);
    free(A); free(B); free(C);
    
    return result;
}

void nova_autocal_bench_matmul(nova_autocal_context_t *ctx) {
    printf("=== Matrix Multiplication Auto-Calibration ===\n\n");
    
    MatmulConfig configs[] = {
        // Small square
        {512, 512, 512, 32, 32, 32},
        // Medium square
        {1024, 1024, 1024, 64, 64, 64},
        // Large square
        {2048, 2048, 2048, 128, 128, 128},
        // Tall skinny
        {4096, 256, 256, 128, 32, 32},
        // Short wide
        {256, 4096, 256, 32, 128, 32},
    };
    
    size_t num_configs = sizeof(configs) / sizeof(configs[0]);
    
    for (size_t i = 0; i < num_configs; i++) {
        MatmulConfig *cfg = &configs[i];
        
        printf("Config %zu: M=%zu, N=%zu, K=%zu, Tile=(%zu,%zu,%zu)\n",
               i + 1, cfg->M, cfg->N, cfg->K, cfg->tile_m, cfg->tile_n, cfg->tile_k);
        
        MatmulResult res = bench_matmul(cfg, 50);
        
        printf("  Time: %.3f ms\n", res.time_ms);
        printf("  Throughput: %.2f GFLOPS\n", res.gflops);
        printf("  Efficiency: %.1f%%\n\n", res.efficiency_percent);
        
        char key[256];
        snprintf(key, sizeof(key), "matmul_%zux%zux%zu", cfg->M, cfg->N, cfg->K);
        nova_autocal_record_metric(ctx, key, "time_ms", res.time_ms);
        nova_autocal_record_metric(ctx, key, "gflops", res.gflops);
    }
    
    printf("Matmul calibration complete.\n");
}
