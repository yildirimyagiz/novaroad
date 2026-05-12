/*
 * nova_bench.c — Benchmark Harness
 *
 * Benchmarks:
 *  1. MatMul scaling (M=N=K: 128 → 2048)
 *  2. Conv (simulated as matmul via im2col sizing)
 *  3. Attention pattern (QK^T → softmax → V)
 *  4. Memory-bound elementwise microbench
 *  5. Kernel overhead (many tiny ops vs one fused op)
 *
 * Build:  cc -O2 -std=c11 -o nova_bench nova_bench.c \
 *             nova_graph.c nova_fusion.c nova_memory.c \
 *             nova_scheduler.c nova_backend.c nova_trace.c -lm
 * Run:    ./nova_bench
 */

#include "nova.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* =========================================================================
 * Utilities
 * ========================================================================= */

#define BENCH_REPS 5

static void fill_random(float *data, size_t count) {
  for (size_t i = 0; i < count; ++i)
    data[i] = (float)(rand() % 1000 - 500) / 500.0f;
}

static uint64_t median_ns(uint64_t *samples, int n) {
  /* Simple insertion sort */
  for (int i = 1; i < n; ++i) {
    uint64_t k = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > k) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = k;
  }
  return samples[n / 2];
}

/* =========================================================================
 * 1. MatMul Scaling Benchmark
 * ========================================================================= */

static void bench_matmul(void) {
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  MatMul Scaling Benchmark (M=N=K, f32, no fusion)\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  %-8s  %-12s  %-12s  %-10s\n", "Size", "Median(ms)", "GFLOPS",
         "GB/s");
  printf("  %-8s  %-12s  %-12s  %-10s\n", "----", "----------", "------",
         "----");

  static const int SIZES[] = {128, 256, 512, 1024, 2048};

  for (int si = 0; si < (int)(sizeof(SIZES) / sizeof(SIZES[0])); ++si) {
    int M = SIZES[si];

    NovaGraph *g = nova_graph_create("matmul_bench", 8, 8);
    g->backend = nova_backend_cpu();

    int64_t sh[2] = {M, M};
    uint32_t tA = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh, true);
    uint32_t tB = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh, true);
    uint32_t tC = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh, false);

    uint32_t ins[2] = {tA, tB};
    uint32_t outs[1] = {tC};
    nova_node_add(g, NOVA_OP_MATMUL, "mm", ins, 2, outs, 1);

    nova_graph_optimize(g);
    nova_memory_allocate(g);

    fill_random((float *)g->tensors[tA].data, (size_t)M * M);
    fill_random((float *)g->tensors[tB].data, (size_t)M * M);

    uint64_t samples[BENCH_REPS];
    for (int r = 0; r < BENCH_REPS; ++r) {
      /* Reset executed flags */
      for (uint32_t i = 0; i < g->num_nodes; ++i)
        g->nodes[i].executed = false;

      uint64_t t0 = nova_clock_ns();
      g->backend->execute(g->backend, &g->nodes[0], g->tensors);
      samples[r] = nova_clock_ns() - t0;
    }

    uint64_t med = median_ns(samples, BENCH_REPS);
    double ms = med / 1e6;
    double gf = 2.0 * M * M * M / (double)med;             /* 2*M³ FLOPs */
    double gb = 3.0 * M * M * sizeof(float) / (double)med; /* 3 matrices */

    printf("  %-8d  %-12.3f  %-12.3f  %-10.3f\n", M, ms, gf, gb);

    nova_graph_destroy(g);
  }
  printf("\n");
}

/* =========================================================================
 * 2. Fusion Overhead Benchmark
 *    Compare: separate matmul+bias+relu vs fused version
 * ========================================================================= */

static void bench_fusion(void) {
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  Kernel Fusion Benchmark (matmul → bias → relu)\n");
  printf("  M=N=K=512, f32, %d repetitions\n", BENCH_REPS);
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

  int M = 512;
  int64_t sh2[2] = {M, M};
  int64_t sh1[1] = {M};

  /* --- Unfused graph: matmul → bias_add → relu (3 nodes) --- */
  {
    NovaGraph *g = nova_graph_create("unfused", 16, 16);
    g->backend = nova_backend_cpu();
    g->optimized = true; /* skip fusion for unfused baseline */

    uint32_t tA = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, true);
    uint32_t tB = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, true);
    uint32_t tTmp = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, false);
    uint32_t tBias = nova_tensor_add(g, NOVA_DTYPE_F32, 1, sh1, true);
    uint32_t tTmp2 = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, false);
    uint32_t tOut = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, false);

    uint32_t mm_in[2] = {tA, tB};
    uint32_t mm_out[1] = {tTmp};
    uint32_t ba_in[2] = {tTmp, tBias};
    uint32_t ba_out[1] = {tTmp2};
    uint32_t rel_in[1] = {tTmp2};
    uint32_t rel_out[1] = {tOut};

    nova_node_add(g, NOVA_OP_MATMUL, "mm", mm_in, 2, mm_out, 1);
    nova_node_add(g, NOVA_OP_BIAS_ADD, "bias", ba_in, 2, ba_out, 1);
    nova_node_add(g, NOVA_OP_RELU, "relu", rel_in, 1, rel_out, 1);

    /* Manual topo sort (already linear) */
    g->topo_order[0] = 0;
    g->topo_order[1] = 1;
    g->topo_order[2] = 2;
    g->topo_len = 3;
    for (uint32_t i = 0; i < 3; ++i)
      g->nodes[i].topo_order = (int32_t)i;
    g->memory_planned = true;

    nova_memory_allocate(g);
    fill_random((float *)g->tensors[tA].data, (size_t)M * M);
    fill_random((float *)g->tensors[tB].data, (size_t)M * M);
    fill_random((float *)g->tensors[tBias].data, (size_t)M);

    uint64_t samples[BENCH_REPS];
    for (int r = 0; r < BENCH_REPS; ++r) {
      for (uint32_t i = 0; i < g->num_nodes; ++i)
        g->nodes[i].executed = false;
      uint64_t t0 = nova_clock_ns();
      nova_graph_execute(g);
      samples[r] = nova_clock_ns() - t0;
    }
    printf("  Unfused  (3 kernel launches): median %.3f ms  launches=%u\n",
           median_ns(samples, BENCH_REPS) / 1e6, g->kernel_launches);
    nova_graph_destroy(g);
  }

  /* --- Fused graph: let optimizer fuse all three --- */
  {
    NovaGraph *g = nova_graph_create("fused", 16, 16);
    g->backend = nova_backend_cpu();

    uint32_t tA = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, true);
    uint32_t tB = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, true);
    uint32_t tTmp = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, false);
    uint32_t tBias = nova_tensor_add(g, NOVA_DTYPE_F32, 1, sh1, true);
    uint32_t tTmp2 = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, false);
    uint32_t tOut = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh2, false);

    uint32_t mm_in[2] = {tA, tB};
    uint32_t mm_out[1] = {tTmp};
    uint32_t ba_in[2] = {tTmp, tBias};
    uint32_t ba_out[1] = {tTmp2};
    uint32_t rel_in[1] = {tTmp2};
    uint32_t rel_out[1] = {tOut};

    nova_node_add(g, NOVA_OP_MATMUL, "mm", mm_in, 2, mm_out, 1);
    nova_node_add(g, NOVA_OP_BIAS_ADD, "bias", ba_in, 2, ba_out, 1);
    nova_node_add(g, NOVA_OP_RELU, "relu", rel_in, 1, rel_out, 1);

    nova_graph_optimize(g); /* fusion happens here */
    nova_memory_allocate(g);
    fill_random((float *)g->tensors[tA].data, (size_t)M * M);
    fill_random((float *)g->tensors[tB].data, (size_t)M * M);
    if (g->tensors[tBias].data)
      fill_random((float *)g->tensors[tBias].data, (size_t)M);

    uint64_t samples[BENCH_REPS];
    for (int r = 0; r < BENCH_REPS; ++r) {
      for (uint32_t i = 0; i < g->num_nodes; ++i)
        g->nodes[i].executed = false;
      uint64_t t0 = nova_clock_ns();
      nova_graph_execute(g);
      samples[r] = nova_clock_ns() - t0;
    }
    printf("  Fused    (%u kernel launches): median %.3f ms  fusions=%u\n",
           g->kernel_launches, median_ns(samples, BENCH_REPS) / 1e6,
           g->fusions_applied);
    nova_graph_destroy(g);
  }
  printf("\n");
}

/* =========================================================================
 * 3. Attention Pattern Benchmark (Q·K^T → softmax → ·V)
 * ========================================================================= */

static void bench_attention(void) {
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  Attention Pattern Benchmark\n");
  printf("  seq_len=512, d_model=64, f32\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

  int S = 512, D = 64;
  NovaGraph *g = nova_graph_create("attention", 32, 32);
  g->backend = nova_backend_cpu();

  int64_t qkv_sh[2] = {S, D};
  int64_t att_sh[2] = {S, S};

  uint32_t tQ = nova_tensor_add(g, NOVA_DTYPE_F32, 2, qkv_sh, true);
  uint32_t tK = nova_tensor_add(g, NOVA_DTYPE_F32, 2, qkv_sh, true);
  uint32_t tV = nova_tensor_add(g, NOVA_DTYPE_F32, 2, qkv_sh, true);
  uint32_t tQK = nova_tensor_add(g, NOVA_DTYPE_F32, 2, att_sh, false);
  uint32_t tSM = nova_tensor_add(g, NOVA_DTYPE_F32, 2, att_sh, false);
  uint32_t tOut = nova_tensor_add(g, NOVA_DTYPE_F32, 2, qkv_sh, false);

  uint32_t qk_in[2] = {tQ, tK};
  uint32_t qk_out[1] = {tQK};
  uint32_t sm_in[1] = {tQK};
  uint32_t sm_out[1] = {tSM};
  uint32_t av_in[2] = {tSM, tV};
  uint32_t av_out[1] = {tOut};

  nova_node_add(g, NOVA_OP_MATMUL, "QK", qk_in, 2, qk_out, 1);
  nova_node_add(g, NOVA_OP_SOFTMAX, "softmax", sm_in, 1, sm_out, 1);
  nova_node_add(g, NOVA_OP_MATMUL, "AV", av_in, 2, av_out, 1);

  nova_graph_optimize(g);
  nova_memory_allocate(g);

  fill_random((float *)g->tensors[tQ].data, (size_t)S * D);
  fill_random((float *)g->tensors[tK].data, (size_t)S * D);
  fill_random((float *)g->tensors[tV].data, (size_t)S * D);

  uint64_t samples[BENCH_REPS];
  for (int r = 0; r < BENCH_REPS; ++r) {
    for (uint32_t i = 0; i < g->num_nodes; ++i)
      g->nodes[i].executed = false;
    uint64_t t0 = nova_clock_ns();
    nova_graph_execute(g);
    samples[r] = nova_clock_ns() - t0;
  }
  printf("  Median: %.3f ms, launches=%u, fusions=%u\n\n",
         median_ns(samples, BENCH_REPS) / 1e6, g->kernel_launches,
         g->fusions_applied);

  nova_graph_destroy(g);
}

/* =========================================================================
 * 4. Memory-bound elementwise microbench
 * ========================================================================= */

static void bench_elementwise(void) {
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  Memory-Bound Elementwise Benchmark (relu chain)\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

  static const size_t ELEM_COUNTS[] = {1 << 18, 1 << 20, 1 << 22, 1 << 24};

  for (int ei = 0; ei < 4; ++ei) {
    size_t N = ELEM_COUNTS[ei];

    NovaGraph *g = nova_graph_create("elementwise", 8, 8);
    g->backend = nova_backend_cpu();

    int64_t sh[1] = {(int64_t)N};
    uint32_t t0 = nova_tensor_add(g, NOVA_DTYPE_F32, 1, sh, true);
    uint32_t t1 = nova_tensor_add(g, NOVA_DTYPE_F32, 1, sh, false);

    uint32_t ins[1] = {t0};
    uint32_t outs[1] = {t1};
    nova_node_add(g, NOVA_OP_RELU, "relu", ins, 1, outs, 1);

    nova_graph_optimize(g);
    nova_memory_allocate(g);
    fill_random((float *)g->tensors[t0].data, N);

    uint64_t samples[BENCH_REPS];
    for (int r = 0; r < BENCH_REPS; ++r) {
      g->nodes[0].executed = false;
      uint64_t ts = nova_clock_ns();
      g->backend->execute(g->backend, &g->nodes[0], g->tensors);
      samples[r] = nova_clock_ns() - ts;
    }

    uint64_t med = median_ns(samples, BENCH_REPS);
    double gb = 2.0 * N * sizeof(float) / (double)med;
    printf("  N=%-10zu  median=%.3f ms  bandwidth=%.2f GB/s\n", N, med / 1e6,
           gb);

    nova_graph_destroy(g);
  }
  printf("\n");
}

/* =========================================================================
 * 5. Memory Reuse Benchmark
 * ========================================================================= */

static void bench_memory_reuse(void) {
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  Memory Planner — Buffer Reuse Stats\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

  /* Linear chain: t0→op0→t1→op1→t2→op2→t3 (t1 and t3 can alias) */
  int M = 512;
  int64_t sh[2] = {M, M};

  NovaGraph *g = nova_graph_create("reuse_test", 16, 16);
  g->backend = nova_backend_cpu();

  uint32_t t[5];
  for (int i = 0; i < 5; ++i)
    t[i] = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh, i == 0);

  for (int i = 0; i < 4; ++i) {
    uint32_t ins[1] = {t[i]};
    uint32_t outs[1] = {t[i + 1]};
    char name[16];
    snprintf(name, sizeof(name), "relu_%d", i);
    nova_node_add(g, NOVA_OP_RELU, name, ins, 1, outs, 1);
  }

  nova_graph_optimize(g);
  nova_memory_plan(g); /* prints reuse stats */

  uint32_t aliases = 0;
  for (uint32_t i = 0; i < g->num_tensors; ++i)
    if (g->tensors[i].is_alias)
      aliases++;
  printf("  Tensors aliased: %u / %u\n\n", aliases, g->num_tensors);

  nova_graph_destroy(g);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void) {
  srand(42);

  printf("\n");
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("║          Nova Execution Efficiency Layer — Benchmarks      ║\n");
  printf(
      "╚══════════════════════════════════════════════════════════════╝\n\n");

  bench_matmul();
  bench_fusion();
  bench_attention();
  bench_elementwise();
  bench_memory_reuse();

  printf("All benchmarks complete.\n");
  return 0;
}
