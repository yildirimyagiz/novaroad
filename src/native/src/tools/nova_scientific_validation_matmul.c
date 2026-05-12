/**
 * ═══════════════════════════════════════════════════════════════════════════
 * SECTION 1: KERNEL-LEVEL MICROBENCHMARKS - MATRIX MULTIPLICATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_scientific_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
// FP32 MATRIX MULTIPLICATION - NAIVE SCALAR
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_fp32_naive(const float* A, const float* B, float* C, 
                              int M, int N, int K) {
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

// ═══════════════════════════════════════════════════════════════════════════
// FP32 MATRIX MULTIPLICATION - SIMD OPTIMIZED
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_fp32_simd(const float* A, const float* B, float* C, 
                             int M, int N, int K) {
#ifdef __ARM_NEON
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float32x4_t sum_vec = vdupq_n_f32(0.0f);
            int k;
            
            // Process 4 elements at a time
            for (k = 0; k + 4 <= K; k += 4) {
                float32x4_t a_vec = vld1q_f32(&A[i * K + k]);
                float32x4_t b_vec = vld1q_f32(&B[k * N + j]);
                sum_vec = vmlaq_f32(sum_vec, a_vec, b_vec);
            }
            
            // Horizontal sum
            float sum = vaddvq_f32(sum_vec);
            
            // Handle remaining elements
            for (; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            
            C[i * N + j] = sum;
        }
    }
#else
    // Fallback to naive if NEON not available
    matmul_fp32_naive(A, B, C, M, N, K);
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// FP32 MATRIX MULTIPLICATION - REGISTER TILED
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_fp32_tiled(const float* A, const float* B, float* C, 
                              int M, int N, int K) {
    const int TILE_M = 8;
    const int TILE_N = 8;
    const int TILE_K = 64;
    
    // Initialize output
    memset(C, 0, M * N * sizeof(float));
    
    for (int i0 = 0; i0 < M; i0 += TILE_M) {
        for (int j0 = 0; j0 < N; j0 += TILE_N) {
            for (int k0 = 0; k0 < K; k0 += TILE_K) {
                // Process tile
                int i_max = (i0 + TILE_M < M) ? i0 + TILE_M : M;
                int j_max = (j0 + TILE_N < N) ? j0 + TILE_N : N;
                int k_max = (k0 + TILE_K < K) ? k0 + TILE_K : K;
                
                for (int i = i0; i < i_max; i++) {
                    for (int j = j0; j < j_max; j++) {
                        float sum = C[i * N + j];
                        for (int k = k0; k < k_max; k++) {
                            sum += A[i * K + k] * B[k * N + j];
                        }
                        C[i * N + j] = sum;
                    }
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// FP32 MATRIX MULTIPLICATION - FUSED (MatMul + Bias + ReLU)
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_fp32_fused(const float* A, const float* B, float* C,
                              const float* bias, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            // Fuse bias add and ReLU activation
            sum += bias[j];
            C[i * N + j] = (sum > 0.0f) ? sum : 0.0f;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// INT8 MATRIX MULTIPLICATION - NAIVE
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_int8_naive(const int8_t* A, const int8_t* B, int32_t* C, 
                              int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int32_t sum = 0;
            for (int k = 0; k < K; k++) {
                sum += (int32_t)A[i * K + k] * (int32_t)B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// INT8 MATRIX MULTIPLICATION - SIMD OPTIMIZED
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_int8_simd(const int8_t* A, const int8_t* B, int32_t* C, 
                             int M, int N, int K) {
#ifdef __ARM_NEON
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int32x4_t sum_vec = vdupq_n_s32(0);
            int k;
            
            // Process 8 elements at a time
            for (k = 0; k + 8 <= K; k += 8) {
                int8x8_t a_vec = vld1_s8(&A[i * K + k]);
                int8x8_t b_vec = vld1_s8(&B[k * N + j]);
                
                // Widen to int16 and multiply
                int16x8_t a_wide = vmovl_s8(a_vec);
                int16x8_t b_wide = vmovl_s8(b_vec);
                int16x8_t prod = vmulq_s16(a_wide, b_wide);
                
                // Accumulate to int32
                sum_vec = vaddw_s16(sum_vec, vget_low_s16(prod));
                sum_vec = vaddw_s16(sum_vec, vget_high_s16(prod));
            }
            
            // Horizontal sum
            int32_t sum = vaddvq_s32(sum_vec);
            
            // Handle remaining elements
            for (; k < K; k++) {
                sum += (int32_t)A[i * K + k] * (int32_t)B[k * N + j];
            }
            
            C[i * N + j] = sum;
        }
    }
#else
    matmul_int8_naive(A, B, C, M, N, K);
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK DRIVER - FP32
// ═══════════════════════════════════════════════════════════════════════════

BenchmarkMetrics bench_matmul(int M, int N, int K, DataType dtype, MatMulVariant variant) {
    BenchmarkMetrics metrics = {0};
    
    const int WARMUP_ITERS = 5;
    const int BENCH_ITERS = 20;
    
    if (dtype == DTYPE_FP32) {
        // Allocate matrices
        float* A = (float*)malloc(M * K * sizeof(float));
        float* B = (float*)malloc(K * N * sizeof(float));
        float* C = (float*)malloc(M * N * sizeof(float));
        float* bias = (float*)malloc(N * sizeof(float));
        
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
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERS; i++) {
            if (variant == MATMUL_NAIVE_SCALAR) {
                matmul_fp32_naive(A, B, C, M, N, K);
            } else if (variant == MATMUL_SIMD_OPTIMIZED) {
                matmul_fp32_simd(A, B, C, M, N, K);
            } else if (variant == MATMUL_REGISTER_TILED) {
                matmul_fp32_tiled(A, B, C, M, N, K);
            } else if (variant == MATMUL_FUSED) {
                matmul_fp32_fused(A, B, C, bias, M, N, K);
            }
        }
        
        // Benchmark
        NovaTimer timer;
        uint64_t total_ns = 0;
        
        for (int i = 0; i < BENCH_ITERS; i++) {
            nova_timer_start(&timer);
            
            if (variant == MATMUL_NAIVE_SCALAR) {
                matmul_fp32_naive(A, B, C, M, N, K);
            } else if (variant == MATMUL_SIMD_OPTIMIZED) {
                matmul_fp32_simd(A, B, C, M, N, K);
            } else if (variant == MATMUL_REGISTER_TILED) {
                matmul_fp32_tiled(A, B, C, M, N, K);
            } else if (variant == MATMUL_FUSED) {
                matmul_fp32_fused(A, B, C, bias, M, N, K);
            }
            
            nova_timer_stop(&timer);
            total_ns += timer.elapsed_ns;
        }
        
        // Calculate metrics
        double avg_ns = (double)total_ns / (double)BENCH_ITERS;
        metrics.latency_us = avg_ns / 1000.0;
        metrics.latency_ms = avg_ns / 1000000.0;
        
        // GFLOPS calculation: 2*M*N*K operations
        double flops = 2.0 * (double)M * (double)N * (double)K;
        metrics.throughput_gflops = flops / avg_ns;
        
        // Checksum
        metrics.checksum = nova_checksum_fp32(C, M * N);
        metrics.validated = !nova_detect_dead_code_elimination(metrics.checksum);
        metrics.iterations = BENCH_ITERS;
        
        // Anomaly detection
        if (nova_detect_anomaly_latency(metrics.latency_us)) {
            metrics.anomaly_detected = true;
            snprintf(metrics.anomaly_msg, sizeof(metrics.anomaly_msg),
                    "Suspiciously low latency - possible dead code elimination");
        }
        
        free(A);
        free(B);
        free(C);
        free(bias);
        
    } else if (dtype == DTYPE_INT8) {
        // Allocate matrices for INT8
        int8_t* A = (int8_t*)malloc(M * K * sizeof(int8_t));
        int8_t* B = (int8_t*)malloc(K * N * sizeof(int8_t));
        int32_t* C = (int32_t*)malloc(M * N * sizeof(int32_t));
        
        // Initialize
        for (int i = 0; i < M * K; i++) {
            A[i] = (int8_t)(rand() % 256 - 128);
        }
        for (int i = 0; i < K * N; i++) {
            B[i] = (int8_t)(rand() % 256 - 128);
        }
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERS; i++) {
            if (variant == MATMUL_NAIVE_SCALAR) {
                matmul_int8_naive(A, B, C, M, N, K);
            } else {
                matmul_int8_simd(A, B, C, M, N, K);
            }
        }
        
        // Benchmark
        NovaTimer timer;
        uint64_t total_ns = 0;
        
        for (int i = 0; i < BENCH_ITERS; i++) {
            nova_timer_start(&timer);
            
            if (variant == MATMUL_NAIVE_SCALAR) {
                matmul_int8_naive(A, B, C, M, N, K);
            } else {
                matmul_int8_simd(A, B, C, M, N, K);
            }
            
            nova_timer_stop(&timer);
            total_ns += timer.elapsed_ns;
        }
        
        // Calculate metrics
        double avg_ns = (double)total_ns / (double)BENCH_ITERS;
        metrics.latency_us = avg_ns / 1000.0;
        metrics.latency_ms = avg_ns / 1000000.0;
        
        // TOPS calculation for INT8
        double ops = 2.0 * (double)M * (double)N * (double)K;
        metrics.throughput_tops = ops / avg_ns;
        
        // Checksum
        metrics.checksum = nova_checksum_int8(A, M * K) ^ nova_checksum_int8(B, K * N);
        metrics.validated = !nova_detect_dead_code_elimination(metrics.checksum);
        metrics.iterations = BENCH_ITERS;
        
        free(A);
        free(B);
        free(C);
    }
    
    yield metrics;
}
