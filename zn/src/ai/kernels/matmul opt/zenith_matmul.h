#ifndef ZENITH_MATMUL_H
#define ZENITH_MATMUL_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// ─── Constants ────────────────────────────────────────────────
#define CACHE_LINE 64
#define L2_SIZE_KB 512
#define NUM_THREADS 8
#define THRESHOLD_FP32 128
#define THRESHOLD_FP16 512
#define GPU_BM 128
#define GPU_BN 128
#define GPU_SM_COUNT 114

// ─── Types ────────────────────────────────────────────────────
typedef enum {
  BACKEND_CPU_BLOCKED,
  BACKEND_CPU_SIMD,
  BACKEND_GPU_FP32,
  BACKEND_GPU_FP16,
  BACKEND_GPU_INT8
} Backend;

typedef struct {
  int rows, cols;
  float *data;
  __fp16 *data_fp16;
  int8_t *data_int8;
  float scale;
  bool is_aligned;
  bool quantized;
} Matrix;

typedef struct {
  double compute_ms;
  double gflops;
  double energy_j;
  double bandwidth_gb_s;
  int waves;
  int tail_blocks;
  Backend backend_used;
} PerfStats;

// ─── Public API ───────────────────────────────────────────────
Matrix *matrix_create(int rows, int cols);
void matrix_destroy(Matrix *m);
void matrix_fill_random(Matrix *m);
void matrix_quantize_int8(Matrix *m);

void matmul_cpu_blocked(Matrix *A, Matrix *B, volatile Matrix *C);
void matmul_cpu_simd(Matrix *A, Matrix *B, volatile Matrix *C);
void matmul_adaptive(Matrix *A, Matrix *B, volatile Matrix *C,
                     PerfStats *stats);

// Metal Dispatch API
void matmul_gpu_fp32(Matrix *A, Matrix *B, Matrix *C);
void matmul_gpu_fp16(Matrix *A, Matrix *B, Matrix *C);
void matmul_gpu_int8(Matrix *A, Matrix *B, Matrix *C);
void matmul_metal_fp16(Matrix *A, Matrix *B, Matrix *C);

// Benchmark & Helper API
void benchmark_suite(int *sizes, int n_sizes, int runs);
int wave_optimal_blocks(int total_blocks, int sm_count);
double get_time_ms(void);

#endif // ZENITH_MATMUL_H
