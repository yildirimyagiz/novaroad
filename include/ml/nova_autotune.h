/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_autotune.h - Kernel Fusion & Hardware Autotuning
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_AUTOTUNE_H
#define NOVA_AUTOTUNE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nova_tensor.h"

// Forward declarations
typedef struct NovaComputeContext NovaComputeContext;

typedef struct {
  char name[128];
  int core_count;
  int threads_per_core;
  size_t l1_cache_size;
  size_t l2_cache_size;
  size_t l3_cache_size;
  bool has_avx2;
  bool has_avx512;
  bool has_neon;
  bool has_amx;
  bool has_fp16_native;
  bool has_sve;
  int preferred_tile_m;
  int preferred_tile_n;
  int preferred_tile_k;
  NovaBackendType backend;
} NovaHardwareProfile;

typedef struct {
  int tile_m;
  int tile_n;
  int tile_k;
  double time_ms;
  int thread_x;
  int thread_y;
  char kernel_name[128];
} AutotuneResult;

typedef struct {
  double flops;
  double memory_bandwidth;
  double l1_hit_rate;
  double l2_hit_rate;
  double l3_hit_rate;
  int optimal_tile_size;
  int optimal_block_size;
  int optimal_threads;
} HardwareBenchmark;

typedef enum {
  NOVA_SEARCH_RANDOM,
  NOVA_SEARCH_GRID,
  NOVA_SEARCH_BAYESIAN,    // Optuna style
  NOVA_SEARCH_EVOLUTIONARY // Nevergrad style
} NovaSearchStrategy;

typedef struct {
  NovaHardwareProfile profile;
  HardwareBenchmark hw_bench;
  NovaComputeContext *ctx;
  NovaSearchStrategy strategy;
  bool use_cache;
  char cache_path[256];

  // Stats for the "Self-Learning" feedback loop
  uint64_t total_trials;
  double best_cost;
} NovaAutotuner;

NovaAutotuner *nova_autotuner_create(NovaComputeContext *ctx);
void nova_autotuner_destroy(NovaAutotuner *at);
void nova_autotune_detect_arch(NovaAutotuner *at);
HardwareBenchmark nova_benchmark_hardware(NovaHardwareProfile *profile);
double nova_benchmark_peak_flops(NovaHardwareProfile *profile);
double nova_benchmark_memory_bandwidth(void);
void *nova_autotune_get_best_kernel(NovaAutotuner *at, const char *op,
                                      int m, int n, int k);

// Runtime-tuned configuration (populated by autotuner, queried by kernels)
typedef struct {
  int matmul_tile_m;
  int matmul_tile_n;
  int matmul_tile_k;
  int attn_tile_size;
  int prefetch_distance;
  bool is_tuned;
} NovaTunedConfig;

const NovaTunedConfig *nova_get_tuned_config(void);
void nova_autotune_matmul(NovaAutotuner *at);
void nova_autotune_attention(NovaAutotuner *at);
void nova_autotune_print_config(const NovaAutotuner *at);

#endif // NOVA_AUTOTUNE_H
