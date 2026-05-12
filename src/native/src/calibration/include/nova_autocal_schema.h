// 🦅 Novaign Calibration — Result Schema
// Covers CPU + GPU optimized configurations per platform.

#pragma once

#include "nova_autocal_hw.h"
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// CPU CALIBRATION RESULT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // MatMul tile strategy
  int matmul_tile_m;
  int matmul_tile_n;
  int matmul_tile_k;
  int matmul_prefetch_distance;

  // Attention (Transformer) tiles
  int attn_tile_size;
  int attn_head_parallelism;

  // Thread pool
  int worker_threads;
  bool use_smt;        // Use hyper-threading?
  bool pin_perf_cores; // Pin to P-cores on big.LITTLE?

  // Measured performance
  double measured_gflops;
  double measured_bandwidth_gb_s;
} NovaCPUCalibration;

// ═══════════════════════════════════════════════════════════════════════════
// GPU CALIBRATION RESULT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaGPUBackend backend;
  bool calibrated;

  // Threadgroup / block dimensions
  int threadgroup_x;
  int threadgroup_y;
  int block_size_1d; // For vector ops

  // MatMul strategy
  int gpu_tile_m;
  int gpu_tile_n;
  int gpu_tile_k;
  bool use_shared_memory;
  bool use_fp16_accumulate; // Mixed precision

  // Measured
  double measured_tflops;
  double measured_bandwidth_gb_s;
} NovaGPUCalibration;

// ═══════════════════════════════════════════════════════════════════════════
// FULL CALIBRATION SCHEMA (Machine-wide)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // Metadata
  bool is_calibrated;
  uint64_t calibration_timestamp;
  NovaTargetOS target_os;
  NovaArchType target_arch;

  // Per-device results
  NovaCPUCalibration cpu;
  NovaGPUCalibration gpus[NOVA_MAX_GPUS];
  int gpu_count;

  // Recommended primary compute
  int preferred_gpu_index; // -1 = CPU preferred
} NovaCalibrationSchema;

/// Global calibration config — queried by all kernels at runtime.
extern NovaCalibrationSchema g_nova_sovereign_config;

/// Load/Save calibration from disk cache.
bool nova_load_calibration(const char *path, NovaCalibrationSchema *out);
bool nova_save_calibration(const char *path, const NovaCalibrationSchema *in);
