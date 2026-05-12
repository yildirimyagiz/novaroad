#include "nova_autotune.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL TUNED CONFIG (Queried by kernels at runtime)
// ═══════════════════════════════════════════════════════════════════════════

static NovaTunedConfig g_tuned_config = {
    .matmul_tile_m = 8,
    .matmul_tile_n = 4,
    .matmul_tile_k = 64,
    .attn_tile_size = 32,
    .prefetch_distance = 4,
    .is_tuned = false,
};

const NovaTunedConfig *nova_get_tuned_config(void) {
  return &g_tuned_config;
}

// ═══════════════════════════════════════════════════════════════════════════
// HIGH-RES TIMER
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t autotune_now_ns(void) {
#ifdef __APPLE__
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// HARDWARE DETECTION
// ═══════════════════════════════════════════════════════════════════════════

static void detect_hardware_profile(NovaAutotuner *at) {
  NovaHardwareProfile *p = &at->profile;
  memset(p, 0, sizeof(NovaHardwareProfile));

#ifdef __APPLE__
  {
    size_t sz;
    char model[256] = {0};
    sz = sizeof(model);
    if (sysctlbyname("hw.model", model, &sz, NULL, 0) == 0)
      strncpy(p->name, model, sizeof(p->name) - 1);

    int64_t val;
    sz = sizeof(val);
    if (sysctlbyname("hw.ncpu", &val, &sz, NULL, 0) == 0)
      p->core_count = (int)val;
    p->threads_per_core = 1;

    if (sysctlbyname("hw.l1dcachesize", &val, &sz, NULL, 0) == 0)
      p->l1_cache_size = (size_t)val;
    if (sysctlbyname("hw.l2cachesize", &val, &sz, NULL, 0) == 0)
      p->l2_cache_size = (size_t)val;
    if (sysctlbyname("hw.l3cachesize", &val, &sz, NULL, 0) == 0)
      p->l3_cache_size = (size_t)val;
  }

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  p->has_neon = true;
#endif
#endif

  // Compute cache-aware default tile sizes
  // L1 typically 32-64KB. For a tile of M*K*sizeof(float) to fit in L1:
  // M*K*4 <= L1/2 (leave room for B tile)
  int l1_kb = p->l1_cache_size > 0 ? (int)(p->l1_cache_size / 1024) : 32;
  if (l1_kb >= 64) {
    p->preferred_tile_m = 8;
    p->preferred_tile_k = 64;
  } else {
    p->preferred_tile_m = 4;
    p->preferred_tile_k = 32;
  }
  p->preferred_tile_n = 4; // NEON width
}

// ═══════════════════════════════════════════════════════════════════════════
// MICROKERNEL BENCHMARK (Parametric matmul tile test)
// ═══════════════════════════════════════════════════════════════════════════

static double bench_matmul_config(int tile_m, int tile_k, int prefetch_dist,
                                  int M, int K, int N, const float *A,
                                  const float *B, float *C) {
  // Warmup
  for (int w = 0; w < 3; w++) {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0;
        for (int p = 0; p < K; p++)
          sum += A[i * K + p] * B[p * N + j];
        C[i * N + j] = sum;
      }
    }
  }

  // Timed run with specific tile config
  const int ITERS = 50;
  uint64_t best = UINT64_MAX;

  for (int it = 0; it < ITERS; it++) {
    memset(C, 0, M * N * sizeof(float));
    uint64_t t0 = autotune_now_ns();

    // Simulate tiled matmul with given parameters
    for (int i0 = 0; i0 < M; i0 += tile_m) {
      int i_end = (i0 + tile_m < M) ? i0 + tile_m : M;
      for (int k0 = 0; k0 < K; k0 += tile_k) {
        int k_end = (k0 + tile_k < K) ? k0 + tile_k : K;
        for (int j = 0; j < N; j++) {
          for (int i = i0; i < i_end; i++) {
            float sum = C[i * N + j];
            for (int p = k0; p < k_end; p++) {
              if (p + prefetch_dist < k_end)
                __builtin_prefetch(&B[(p + prefetch_dist) * N + j], 0, 1);
              sum += A[i * K + p] * B[p * N + j];
            }
            C[i * N + j] = sum;
          }
        }
      }
    }

    uint64_t elapsed = autotune_now_ns() - t0;
    if (elapsed < best)
      best = elapsed;
  }

  return (double)best;
}

// ═══════════════════════════════════════════════════════════════════════════
// RUNTIME AUTOTUNING (Called once at engine init)
// ═══════════════════════════════════════════════════════════════════════════

void nova_autotune_matmul(NovaAutotuner *at) {
  // Test matrix size (representative of real workloads)
  const int M = 64, K = 128, N = 64;
  float *A = malloc(M * K * sizeof(float));
  float *B = malloc(K * N * sizeof(float));
  float *C = malloc(M * N * sizeof(float));

  for (int i = 0; i < M * K; i++)
    A[i] = (float)(i % 17) * 0.1f;
  for (int i = 0; i < K * N; i++)
    B[i] = (float)(i % 13) * 0.1f;

  // Search space: tile_m x tile_k x prefetch_distance
  int tile_ms[] = {4, 8, 16};
  int tile_ks[] = {16, 32, 64, 128};
  int prefetches[] = {2, 4, 8};

  int n_tm = sizeof(tile_ms) / sizeof(tile_ms[0]);
  int n_tk = sizeof(tile_ks) / sizeof(tile_ks[0]);
  int n_pf = sizeof(prefetches) / sizeof(prefetches[0]);

  double best_time = DBL_MAX;
  int best_tm = 8, best_tk = 64, best_pf = 4;

  at->total_trials = 0;

  for (int itm = 0; itm < n_tm; itm++) {
    for (int itk = 0; itk < n_tk; itk++) {
      for (int ipf = 0; ipf < n_pf; ipf++) {
        double t = bench_matmul_config(tile_ms[itm], tile_ks[itk],
                                       prefetches[ipf], M, K, N, A, B, C);
        at->total_trials++;
        if (t < best_time) {
          best_time = t;
          best_tm = tile_ms[itm];
          best_tk = tile_ks[itk];
          best_pf = prefetches[ipf];
        }
      }
    }
  }

  // Store result globally
  g_tuned_config.matmul_tile_m = best_tm;
  g_tuned_config.matmul_tile_k = best_tk;
  g_tuned_config.prefetch_distance = best_pf;
  g_tuned_config.is_tuned = true;
  at->best_cost = best_time;

  // Update hardware profile
  at->profile.preferred_tile_m = best_tm;
  at->profile.preferred_tile_k = best_tk;

  free(A);
  free(B);
  free(C);
}

// ═══════════════════════════════════════════════════════════════════════════
// ATTENTION TILE AUTOTUNING
// ═══════════════════════════════════════════════════════════════════════════

void nova_autotune_attention(NovaAutotuner *at) {
  int tile_sizes[] = {16, 32, 64, 128};
  int n_tiles = sizeof(tile_sizes) / sizeof(tile_sizes[0]);

  // Simple heuristic: pick tile size based on L1 cache
  // For D=64, each KV row = 256 bytes. Tile of 32 rows = 8KB. Fits in L1.
  int l1_kb = at->profile.l1_cache_size > 0
                  ? (int)(at->profile.l1_cache_size / 1024)
                  : 32;

  int best_tile = 32; // safe default
  for (int i = 0; i < n_tiles; i++) {
    // Each tile needs: tile_size * D * 4 bytes for K + same for V + scores
    int mem_needed_kb = (tile_sizes[i] * 64 * 4 * 2 + tile_sizes[i] * 4) / 1024;
    if (mem_needed_kb < l1_kb / 2) {
      best_tile = tile_sizes[i]; // largest that fits in half L1
    }
  }

  g_tuned_config.attn_tile_size = best_tile;
}

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════

NovaAutotuner *nova_autotuner_create(NovaComputeContext *ctx) {
  NovaAutotuner *at = (NovaAutotuner *)calloc(1, sizeof(NovaAutotuner));
  at->ctx = ctx;
  detect_hardware_profile(at);

  // Run autotuning
  nova_autotune_matmul(at);
  nova_autotune_attention(at);

  // Run standard benchmarks
  at->hw_bench = nova_benchmark_hardware(&at->profile);
  at->hw_bench.optimal_tile_size = g_tuned_config.matmul_tile_m;

  return at;
}

void nova_autotuner_destroy(NovaAutotuner *at) { free(at); }

void nova_autotune_print_config(const NovaAutotuner *at) {
  const NovaTunedConfig *c = &g_tuned_config;
  printf("\n╔═══════════════════════════════════════════════════════════╗\n");
  printf("║  Nova Autotuner Results                                ║\n");
  printf("╠═══════════════════════════════════════════════════════════╣\n");
  printf("║  Hardware: %-45s  ║\n", at->profile.name);
  printf("║  Cores: %-3d  L1: %zuKB  L2: %zuKB                        ║\n",
         at->profile.core_count, at->profile.l1_cache_size / 1024,
         at->profile.l2_cache_size / 1024);
  printf("╠═══════════════════════════════════════════════════════════╣\n");
  printf("║  MatMul Tile-M: %-3d  Tile-K: %-3d  Prefetch: %-2d          ║\n",
         c->matmul_tile_m, c->matmul_tile_k, c->prefetch_distance);
  printf("║  Attention Tile: %-3d                                     ║\n",
         c->attn_tile_size);
  printf("║  Trials: %-4llu  Best: %.1f ns                             ║\n",
         at->total_trials, at->best_cost);
  printf("║  FLOPS: %.2f GFLOPS  BW: %.2f GB/s                      ║\n",
         at->hw_bench.flops / 1e9, at->hw_bench.memory_bandwidth);
  printf("╚═══════════════════════════════════════════════════════════╝\n\n");
}

// Peak FLOPS benchmark: c = a + b * c
double nova_benchmark_peak_flops(NovaHardwareProfile *profile) {
  (void)profile;
  const int N = 1024 * 1024;
  float *a = malloc(N * sizeof(float));
  float *b = malloc(N * sizeof(float));
  float *c = malloc(N * sizeof(float));
  for (int i = 0; i < N; i++) {
    a[i] = 1.0f;
    b[i] = 2.0f;
    c[i] = 0.0f;
  }

  struct timeval start, end;
  gettimeofday(&start, NULL);
  for (int iter = 0; iter < 100; iter++) {
    for (int i = 0; i < N; i++) {
      c[i] = a[i] + b[i] * c[i];
    }
  }
  gettimeofday(&end, NULL);
  double elapsed =
      (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
  double tflops = (100.0 * N * 2.0) / elapsed;
  free(a);
  free(b);
  free(c);
  return tflops;
}

double nova_benchmark_memory_bandwidth(void) {
  const size_t SIZE = 64 * 1024 * 1024;
  char *buffer = malloc(SIZE);
  memset(buffer, 0, SIZE);
  struct timeval start, end;
  gettimeofday(&start, NULL);
  volatile char dummy = 0;
  for (size_t i = 0; i < SIZE; i += 64)
    dummy += buffer[i];
  gettimeofday(&end, NULL);
  double elapsed =
      (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
  double gb_s = (SIZE / 1e9) / elapsed;
  free(buffer);
  return gb_s;
}

HardwareBenchmark nova_benchmark_hardware(NovaHardwareProfile *profile) {
  HardwareBenchmark bench = {0};
  bench.flops = nova_benchmark_peak_flops(profile);
  bench.memory_bandwidth = nova_benchmark_memory_bandwidth();
  bench.optimal_tile_size = g_tuned_config.matmul_tile_m;
  bench.optimal_threads = profile->core_count > 0 ? profile->core_count : 1;
  return bench;
}

void *nova_autotune_get_best_kernel(NovaAutotuner *at, const char *op,
                                      int m, int n, int k) {
  (void)at;
  (void)op;
  (void)m;
  (void)n;
  (void)k;
  return NULL;
}
