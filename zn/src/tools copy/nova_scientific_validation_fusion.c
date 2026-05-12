/**
 * ═══════════════════════════════════════════════════════════════════════════
 * SECTION 3: FUSION EFFICIENCY TESTS
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_scientific_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════
// UNFUSED: MatMul → Add → Activation (3 separate kernels)
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_unfused(const float* A, const float* B, float* C, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

static void add_bias_unfused(float* C, const float* bias, int M, int N) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            C[i * N + j] += bias[j];
        }
    }
}

static void relu_unfused(float* C, int M, int N) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float val = C[i * N + j];
            C[i * N + j] = (val > 0.0f) ? val : 0.0f;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// FUSED: MatMul + Add + Activation (1 kernel)
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_add_relu_fused(const float* A, const float* B, float* C,
                                   const float* bias, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            sum += bias[j];
            C[i * N + j] = (sum > 0.0f) ? sum : 0.0f;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GELU ACTIVATION (More complex for fusion testing)
// ═══════════════════════════════════════════════════════════════════════════

static void gelu_unfused(float* C, int M, int N) {
    const float sqrt_2_pi = 0.79788456080286535587989f;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float x = C[i * N + j];
            float cdf = 0.5f * (1.0f + tanhf(sqrt_2_pi * (x + 0.044715f * x * x * x)));
            C[i * N + j] = x * cdf;
        }
    }
}

static void matmul_add_gelu_fused(const float* A, const float* B, float* C,
                                   const float* bias, int M, int N, int K) {
    const float sqrt_2_pi = 0.79788456080286535587989f;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            sum += bias[j];
            // Fused GELU
            float cdf = 0.5f * (1.0f + tanhf(sqrt_2_pi * (sum + 0.044715f * sum * sum * sum)));
            C[i * N + j] = sum * cdf;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK DRIVER
// ═══════════════════════════════════════════════════════════════════════════

FusionMetrics bench_fusion_matmul_add_activation(int M, int N, int K) {
    FusionMetrics metrics = {0};
    
    const int WARMUP_ITERS = 5;
    const int BENCH_ITERS = 20;
    
    // Allocate matrices
    float* A = (float*)malloc(M * K * sizeof(float));
    float* B = (float*)malloc(K * N * sizeof(float));
    float* C_unfused = (float*)malloc(M * N * sizeof(float));
    float* C_fused = (float*)malloc(M * N * sizeof(float));
    float* bias = (float*)malloc(N * sizeof(float));
    
    if (!A || !B || !C_unfused || !C_fused || !bias) {
        fprintf(stderr, "Failed to allocate memory\n");
        free(A); free(B); free(C_unfused); free(C_fused); free(bias);
        return metrics;
    }
    
    // Initialize with non-trivial values
    for (int i = 0; i < M * K; i++) {
        A[i] = (float)(rand() % 100) / 100.0f - 0.5f;
    }
    for (int i = 0; i < K * N; i++) {
        B[i] = (float)(rand() % 100) / 100.0f - 0.5f;
    }
    for (int i = 0; i < N; i++) {
        bias[i] = (float)(rand() % 100) / 100.0f;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // BENCHMARK UNFUSED PATH
    // ═══════════════════════════════════════════════════════════════════════
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; i++) {
        matmul_unfused(A, B, C_unfused, M, N, K);
        add_bias_unfused(C_unfused, bias, M, N);
        gelu_unfused(C_unfused, M, N);
    }
    
    // Benchmark
    NovaTimer timer;
    uint64_t total_ns_unfused = 0;
    
    for (int i = 0; i < BENCH_ITERS; i++) {
        nova_timer_start(&timer);
        
        matmul_unfused(A, B, C_unfused, M, N, K);
        add_bias_unfused(C_unfused, bias, M, N);
        gelu_unfused(C_unfused, M, N);
        
        nova_timer_stop(&timer);
        total_ns_unfused += timer.elapsed_ns;
    }
    
    double avg_ns_unfused = (double)total_ns_unfused / (double)BENCH_ITERS;
    metrics.unfused.latency_us = avg_ns_unfused / 1000.0;
    metrics.unfused.latency_ms = avg_ns_unfused / 1000000.0;
    metrics.unfused.checksum = nova_checksum_fp32(C_unfused, M * N);
    metrics.unfused.validated = !nova_detect_dead_code_elimination(metrics.unfused.checksum);
    metrics.unfused.iterations = BENCH_ITERS;
    
    // GFLOPS calculation
    double flops = 2.0 * (double)M * (double)N * (double)K;
    metrics.unfused.throughput_gflops = flops / avg_ns_unfused;
    
    // ═══════════════════════════════════════════════════════════════════════
    // BENCHMARK FUSED PATH
    // ═══════════════════════════════════════════════════════════════════════
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; i++) {
        matmul_add_gelu_fused(A, B, C_fused, bias, M, N, K);
    }
    
    // Benchmark
    uint64_t total_ns_fused = 0;
    
    for (int i = 0; i < BENCH_ITERS; i++) {
        nova_timer_start(&timer);
        
        matmul_add_gelu_fused(A, B, C_fused, bias, M, N, K);
        
        nova_timer_stop(&timer);
        total_ns_fused += timer.elapsed_ns;
    }
    
    double avg_ns_fused = (double)total_ns_fused / (double)BENCH_ITERS;
    metrics.fused.latency_us = avg_ns_fused / 1000.0;
    metrics.fused.latency_ms = avg_ns_fused / 1000000.0;
    metrics.fused.checksum = nova_checksum_fp32(C_fused, M * N);
    metrics.fused.validated = !nova_detect_dead_code_elimination(metrics.fused.checksum);
    metrics.fused.iterations = BENCH_ITERS;
    
    metrics.fused.throughput_gflops = flops / avg_ns_fused;
    
    // ═══════════════════════════════════════════════════════════════════════
    // CALCULATE FUSION BENEFITS
    // ═══════════════════════════════════════════════════════════════════════
    
    metrics.latency_reduction = (avg_ns_unfused - avg_ns_fused) / avg_ns_unfused;
    metrics.speedup = avg_ns_unfused / avg_ns_fused;
    
    // Memory traffic estimation
    // Unfused: 3 full writes to C (matmul, add, activation)
    // Fused: 1 write to C
    size_t unfused_writes = 3 * M * N * sizeof(float);
    size_t fused_writes = 1 * M * N * sizeof(float);
    metrics.memory_traffic_reduction = (double)(unfused_writes - fused_writes) / (double)unfused_writes;
    
    free(A);
    free(B);
    free(C_unfused);
    free(C_fused);
    free(bias);
    
    return metrics;
}
