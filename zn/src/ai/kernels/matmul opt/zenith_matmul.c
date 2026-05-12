// ============================================================
// NOVA ULTRA MATMUL ENGINE v3.0 — The Core Logic
// Optimized for ARM NEON, Cache-Blocking and Metal Dispatch
// ============================================================

#include "zenith_matmul.h"
#include <arm_neon.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─── Forward Declarations (Implemented in Metal Dispatch) ─────
extern void matmul_gpu_fp16(Matrix *A, Matrix *B, Matrix *C);
extern void matmul_gpu_fp32(Matrix *A, Matrix *B, Matrix *C);
extern void matmul_gpu_int8(Matrix *A, Matrix *B, Matrix *C);

// ─── Matrix Utilities ─────────────────────────────────────────

Matrix *matrix_create(int rows, int cols) {
  Matrix *m = (Matrix *)malloc(sizeof(Matrix));
  m->rows = rows;
  m->cols = cols;
  m->data = (float *)aligned_alloc(64, rows * cols * sizeof(float));
  m->data_int8 = NULL;
  m->scale = 1.0f;
  return m;
}

void matrix_destroy(Matrix *m) {
  if (m) {
    if (m->data)
      free(m->data);
    if (m->data_int8)
      free(m->data_int8);
    free(m);
  }
}

void matrix_fill_random(Matrix *m) {
  for (int i = 0; i < m->rows * m->cols; i++) {
    m->data[i] = (float)rand() / (float)RAND_MAX;
  }
}

// ─── Core Logic Suite ──────────────────────────────────────────

void matmul_cpu_simd(Matrix *A, Matrix *B, volatile Matrix *C) {
  int M = A->rows, K = A->cols, N = B->cols;
  memset(C->data, 0, M * N * sizeof(float));

  // Simple NEON-optimized blocked matmul for CPU fallback
  for (int i = 0; i < M; i++) {
    for (int k = 0; k < K; k++) {
      float32x4_t a_vec = vdupq_n_f32(A->data[i * K + k]);
      for (int j = 0; j < N; j += 4) {
        float32x4_t b_vec = vld1q_f32(&B->data[k * N + j]);
        float32x4_t c_vec = vld1q_f32(&C->data[i * N + j]);
        c_vec = vmlaq_f32(c_vec, a_vec, b_vec);
        vst1q_f32(&C->data[i * N + j], c_vec);
      }
    }
  }
}

void matmul_adaptive(Matrix *A, Matrix *B, volatile Matrix *C, PerfStats *stats) {
  double t0 = get_time_ms();

  int M = A->rows, N = B->cols;
  if (M >= 1024 && N >= 1024) {
    // Use Metal GPU path for large workloads
    matmul_gpu_fp16(A, B, C);
  } else {
    // Use CPU SIMD for small/latency-sensitive workloads
    matmul_cpu_simd(A, B, C);
  }

  double t1 = get_time_ms();
  stats->ms = t1 - t0;
  stats->gflops = (2.0 * A->rows * A->cols * B->cols) / ((t1 - t0) * 1e6);
}

void benchmark_suite(int *sizes, int n_sizes, int runs) {
  printf("\n╔═══════════════════════════════════════════════════════════╗\n");
  printf("║     NOVA ULTRA MATMUL ENGINE v4.0 — THE ORCHESTRATOR   ║\n");
  printf("╚═══════════════════════════════════════════════════════════╝\n\n");

  for (int s = 0; s < n_sizes; s++) {
    int sz = sizes[s];
    Matrix *A = matrix_create(sz, sz);
    Matrix *B = matrix_create(sz, sz);
    matrix_fill_random(A);
    matrix_fill_random(B);

    printf("▶ Size %d×%d:\n", sz, sz);
    for (int r = 0; r < runs; r++) {
      Matrix *C = matrix_create(sz, sz);
      matmul_gpu_fp16(A, B, C); // Direct GPU testing
      matrix_destroy(C);
    }
    printf("\n");
    matrix_destroy(A);
    matrix_destroy(B);
  }
}

// ── Helper functions for energy and wave logic ──

int wave_optimal_blocks(int total, int sm_count) {
  return ((total + sm_count - 1) / sm_count) * sm_count;
}
