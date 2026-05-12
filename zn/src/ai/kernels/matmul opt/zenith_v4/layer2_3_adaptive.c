// ============================================================
// NOVA ULTRA ENGINE v4.0 — LAYER 2-3: Adaptive Motor + Cache/Prefetch
// Backend selection + NEON SIMD kernels
// ============================================================

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <dispatch/dispatch.h>
#include <arm_neon.h>

// ── Constants (from v3.0) ────────────────────────────────────
#define CACHE_LINE          128
#define L1_SIZE_KB          192
#define L2_SIZE_KB          12288
#define CPU_BM              128
#define CPU_BN              128
#define CPU_BK              128
#define CPU_UNROLL          16
#define NUM_THREADS         8

// ── Adaptive Thresholds (auto-tune) ─────────────────────────
#define THRESHOLD_CPU_BLOCKED   128   // ≤128: CPU blocked
#define THRESHOLD_CPU_SIMD      512   // ≤512: CPU SIMD
#define THRESHOLD_GPU_FP16      1024  // ≤1024: GPU FP16
#define THRESHOLD_INT8          2048  // >1024: GPU INT8

// ── Matrix Struct (simplified) ──────────────────────────────
typedef struct {
    float *data;
    int8_t *data_int8;
    float scale;
    float zero_point;
    int rows, cols;
    bool is_aligned;
} Matrix;

// ── Backend Enum ────────────────────────────────────────────
typedef enum {
    BACKEND_CPU_BLOCKED,
    BACKEND_CPU_SIMD,
    BACKEND_GPU_FP16,
    BACKEND_GPU_INT8
} Backend;

// ── Auto-tune EMA ───────────────────────────────────────────
typedef struct {
    double avg_gflops;
    double alpha;  // EMA factor (0.2 = slow adapt)
    int threshold_cpu_blocked;
    int threshold_cpu_simd;
    int threshold_gpu_fp16;
} AutoTune;

// ── Profiling Stats ─────────────────────────────────────────
typedef struct {
    double compute_ms;
    double gflops;
    double energy_j;
    double bandwidth_gb_s;
    Backend backend_used;
    int waves, tail_blocks;
} PerfStats;

// ── Init Matrix ─────────────────────────────────────────────
Matrix* matrix_create(int rows, int cols) {
    Matrix *m = (Matrix*)calloc(1, sizeof(Matrix));
    m->rows = rows; m->cols = cols;
    posix_memalign((void**)&m->data, CACHE_LINE, rows * cols * sizeof(float));
    memset(m->data, 0, rows * cols * sizeof(float));
    m->scale = 1.0f;
    m->zero_point = 0.0f;
    m->is_aligned = true;
    return m;
}

void matrix_destroy(Matrix *m) {
    if (!m) return;
    free(m->data);
    free(m->data_int8);
    free(m);
}

void matrix_fill_random(Matrix *m) {
    for (int i = 0; i < m->rows * m->cols; i++)
        m->data[i] = (float)(rand() % 200 - 100) / 100.0f;
}

// ── Auto-tune Functions ─────────────────────────────────────
void autotune_init(AutoTune *at) {
    at->avg_gflops = 0.0;
    at->alpha = 0.2;
    at->threshold_cpu_blocked = THRESHOLD_CPU_BLOCKED;
    at->threshold_cpu_simd = THRESHOLD_CPU_SIMD;
    at->threshold_gpu_fp16 = THRESHOLD_GPU_FP16;
}

void autotune_update(AutoTune *at, double new_gflops) {
    at->avg_gflops = at->alpha * new_gflops + (1.0 - at->alpha) * at->avg_gflops;

    // Nudge thresholds based on performance
    if (at->avg_gflops < 20.0) {
        at->threshold_cpu_blocked -= 16;
        if (at->threshold_cpu_blocked < 64) at->threshold_cpu_blocked = 64;
    }
    if (at->avg_gflops > 70.0) {
        at->threshold_cpu_simd += 64;
        if (at->threshold_cpu_simd > 1024) at->threshold_cpu_simd = 1024;
    }
}

Backend select_backend_adaptive(int size, AutoTune *at) {
    if (size <= at->threshold_cpu_blocked) return BACKEND_CPU_BLOCKED;
    if (size <= at->threshold_cpu_simd) return BACKEND_CPU_SIMD;
    if (size <= at->threshold_gpu_fp16) return BACKEND_GPU_FP16;
    return BACKEND_GPU_INT8;
}

// ── Timing ──────────────────────────────────────────────────
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// ── CPU Blocked Kernel (L3: Cache/Prefetch) ──────────────────
typedef struct { int start; int end; Matrix *A, *B; volatile Matrix *C; } WorkItem;

static void blocked_worker(Matrix *A, Matrix *B, volatile Matrix *C,
                            int row_start, int row_end) {
    int M = A->rows, K = A->cols, N = B->cols;

    for (int ii = row_start; ii < row_end; ii += CPU_BM) {
    for (int jj = 0; jj < N; jj += CPU_BN) {
    for (int kk = 0; kk < K; kk += CPU_BK) {

        int i_max = (ii + CPU_BM < row_end) ? ii + CPU_BM : row_end;
        int j_max = (jj + CPU_BN < N) ? jj + CPU_BN : N;
        int k_max = (kk + CPU_BK < K) ? kk + CPU_BK : K;

        for (int i = ii; i < i_max; i += 4) {
        for (int j = jj; j < j_max; j += 4) {
            float32x4_t acc0 = vdupq_n_f32(0), acc1 = vdupq_n_f32(0);
            float32x4_t acc2 = vdupq_n_f32(0), acc3 = vdupq_n_f32(0);

            int i_lim = (i+4 < i_max) ? i+4 : i_max;
            int j_lim = (j+4 < j_max) ? j+4 : j_max;

            for (int k = kk; k < k_max; k++) {
                __builtin_prefetch(&A->data[(i)*K + k + 16], 0, 1);
                __builtin_prefetch(&B->data[(k+1)*N + j], 0, 1);

                float32x4_t b_vec = (j_lim - j == 4)
                    ? vld1q_f32(&B->data[k * N + j])
                    : vdupq_n_f32(0);

                if (i_lim - i > 0) acc0 = vmlaq_n_f32(acc0, b_vec, A->data[(i+0)*K+k]);
                if (i_lim - i > 1) acc1 = vmlaq_n_f32(acc1, b_vec, A->data[(i+1)*K+k]);
                if (i_lim - i > 2) acc2 = vmlaq_n_f32(acc2, b_vec, A->data[(i+2)*K+k]);
                if (i_lim - i > 3) acc3 = vmlaq_n_f32(acc3, b_vec, A->data[(i+3)*K+k]);
            }

            if (i_lim - i == 4 && j_lim - j == 4) {
                float *c0 = &C->data[(i+0)*N+j], *c1 = &C->data[(i+1)*N+j];
                float *c2 = &C->data[(i+2)*N+j], *c3 = &C->data[(i+3)*N+j];
                vst1q_f32(c0, vaddq_f32(vld1q_f32(c0), acc0));
                vst1q_f32(c1, vaddq_f32(vld1q_f32(c1), acc1));
                vst1q_f32(c2, vaddq_f32(vld1q_f32(c2), acc2));
                vst1q_f32(c3, vaddq_f32(vld1q_f32(c3), acc3));
            } else {
                float buf[4][4];
                if (i_lim-i>0) vst1q_f32(buf[0], acc0);
                if (i_lim-i>1) vst1q_f32(buf[1], acc1);
                if (i_lim-i>2) vst1q_f32(buf[2], acc2);
                if (i_lim-i>3) vst1q_f32(buf[3], acc3);
                for (int ri=0; ri<i_lim-i; ri++)
                    for (int rj=0; rj<j_lim-j; rj++)
                        C->data[(i+ri)*N+(j+rj)] += buf[ri][rj];
            }
        }}
    }}}
}

void matmul_cpu_blocked(Matrix *A, Matrix *B, volatile Matrix *C) {
    int M = A->rows;
    dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    dispatch_group_t g = dispatch_group_create();
    int chunk = (M + NUM_THREADS - 1) / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; t++) {
        int rs = t * chunk;
        int re = (rs + chunk < M) ? rs + chunk : M;
        if (rs >= M) break;
        dispatch_group_async(g, q, ^{ blocked_worker(A, B, C, rs, re); });
    }
    dispatch_group_wait(g, DISPATCH_TIME_FOREVER);
    dispatch_release(g);
}

// ── CPU SIMD Kernel (L3: Prefetch + Unroll-16) ──────────────
void matmul_cpu_simd(Matrix *A, Matrix *B, volatile Matrix *C) {
    int M = A->rows, K = A->cols, N = B->cols;
    dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    dispatch_group_t g = dispatch_group_create();
    int chunk = (M + NUM_THREADS - 1) / NUM_THREADS;

    for (int t = 0; t < NUM_THREADS; t++) {
        int rs = t * chunk;
        int re = (rs + chunk < M) ? rs + chunk : M;
        if (rs >= M) break;
        dispatch_group_async(g, q, ^{
            for (int i = rs; i < re; i++) {
                for (int j = 0; j <= N - 16; j += 16) {
                    float32x4_t s0=vdupq_n_f32(0), s1=vdupq_n_f32(0);
                    float32x4_t s2=vdupq_n_f32(0), s3=vdupq_n_f32(0);
                    for (int k = 0; k < K; k++) {
                        float a = A->data[i*K+k];
                        float32x4_t av = vdupq_n_f32(a);
                        s0 = vmlaq_f32(s0, av, vld1q_f32(&B->data[k*N+j+ 0]));
                        s1 = vmlaq_f32(s1, av, vld1q_f32(&B->data[k*N+j+ 4]));
                        s2 = vmlaq_f32(s2, av, vld1q_f32(&B->data[k*N+j+ 8]));
                        s3 = vmlaq_f32(s3, av, vld1q_f32(&B->data[k*N+j+12]));
                    }
                    vst1q_f32(&C->data[i*N+j+ 0], vaddq_f32(vld1q_f32(&C->data[i*N+j+ 0]), s0));
                    vst1q_f32(&C->data[i*N+j+ 4], vaddq_f32(vld1q_f32(&C->data[i*N+j+ 4]), s1));
                    vst1q_f32(&C->data[i*N+j+ 8], vaddq_f32(vld1q_f32(&C->data[i*N+j+ 8]), s2));
                    vst1q_f32(&C->data[i*N+j+12], vaddq_f32(vld1q_f32(&C->data[i*N+j+12]), s3));
                }
                // Scalar tail
                for (int j = N - (N%16); j < N; j++) {
                    float s = 0;
                    for (int k = 0; k < K; k++) s += A->data[i*K+k] * B->data[k*N+j];
                    C->data[i*N+j] += s;
                }
            }
        });
    }
    dispatch_group_wait(g, DISPATCH_TIME_FOREVER);
    dispatch_release(g);
}

// ── GPU Dispatch Prototypes (from Metal dispatch) ───────────
void matmul_gpu_fp16(Matrix *A, Matrix *B, Matrix *C);
void matmul_gpu_int8(Matrix *A, Matrix *B, Matrix *C);
