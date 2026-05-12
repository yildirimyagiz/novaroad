#pragma once
// ============================================================
// NOVA ULTRA MATMUL ENGINE v3.0
// Market-Leading: Max GFLOPS + Caching Delta + Min Energy
// Targets: Apple M-series (Metal) + ARM NEON CPU
// Architecture insights: H100-style SM/SMEM/L1/L2 hierarchy
//   adapted for Apple GPU (tile-based deferred renderer)
// ============================================================

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <dispatch/dispatch.h>
#include <arm_neon.h>

// ── Cache / memory constants ──────────────────────────────────
#define CACHE_LINE          128     // Apple M cache line (bytes)
#define L1_SIZE_KB          192     // L1 per core (approx)
#define L2_SIZE_KB          12288   // L2 shared (12 MB)
#define SMEM_PER_BLOCK_KB   32      // GPU threadgroup memory

// ── CPU tiling: fit two blocks (A+B tiles) into L1 ───────────
// L1 = 192KB → tile = sqrt(192KB/2/4bytes) ≈ 154 → use 128
#define CPU_BM              128
#define CPU_BN              128
#define CPU_BK              128
#define CPU_UNROLL          16      // NEON processes 16 floats/cycle
#define NUM_THREADS         8

// ── GPU tiling (Metal) ────────────────────────────────────────
// Match Apple GPU threadgroup = 32KB SMEM
// tile 64×64 FP32 = 64*64*4*2 = 32KB → perfect fit
#define GPU_BM              64
#define GPU_BN              64
#define GPU_BK              16      // K-strip for pipeline overlap
#define GPU_THREADS_X       16
#define GPU_THREADS_Y       16

// ── Wave quantization thresholds ─────────────────────────────
// From wave diagram: avoid tail wave (last wave with 1 block)
// If total_blocks % SM_count == 1 → pad or reshape
#define GPU_SM_COUNT        38      // Apple M2 Ultra GPU cores

// ── Precision thresholds (energy-aware) ──────────────────────
#define THRESHOLD_INT8      1024    // >1024: use INT8
#define THRESHOLD_FP16      512     // 512-1024: use FP16
#define THRESHOLD_FP32      128     // <512: use FP32

// ── Data structures ───────────────────────────────────────────
typedef enum {
    BACKEND_CPU_BLOCKED,
    BACKEND_CPU_SIMD,
    BACKEND_GPU_FP32,
    BACKEND_GPU_FP16,
    BACKEND_GPU_INT8,
} Backend;

typedef struct {
    float   *data;          // aligned FP32
    int16_t *data_fp16;     // FP16 (packed as int16)
    int8_t  *data_int8;     // quantized INT8
    float    scale;         // INT8 quantization scale
    float    zero_point;    // INT8 zero point
    int      rows, cols;
    bool     is_aligned;
    bool     quantized;
} Matrix;

typedef struct {
    double   compute_ms;
    double   gflops;
    double   energy_j;
    double   bandwidth_gb_s;
    Backend  backend_used;
    int      waves;         // number of GPU waves
    int      tail_blocks;   // tail wave blocks (want 0 or full)
} PerfStats;

// ── Public API ────────────────────────────────────────────────
Matrix* matrix_create(int rows, int cols);
void    matrix_destroy(Matrix *m);
void    matrix_quantize_int8(Matrix *m);
void    matrix_fill_random(Matrix *m);

void    matmul_cpu_blocked(Matrix *A, Matrix *B, volatile Matrix *C);
void    matmul_cpu_simd(Matrix *A, Matrix *B, volatile Matrix *C);
void    matmul_adaptive(Matrix *A, Matrix *B, volatile Matrix *C, PerfStats *stats);

// Wave quantization: optimal block count to avoid tail waste
int     wave_optimal_blocks(int total_blocks, int sm_count);

void    benchmark_suite(int *sizes, int n_sizes, int runs);
void    print_perf(PerfStats *s, int M, int N, int K);
