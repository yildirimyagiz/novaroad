// 🦅 Novaign Calibration — Main Engine
// Orchestrates hardware detection, micro-benchmarking, and schema generation.
// Supports: macOS, Linux, Windows, iOS, Android + CPU + GPU.

#include "../../include/nova_autocal.h"
#include "../../include/nova_autocal_hw.h"
#include "../../include/nova_autocal_schema.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

NovaCalibrationSchema g_nova_sovereign_config = {0};

void nova_autocal_run(void) {
  printf("\n🚀 [SOVEREIGN] Starting Full Platform Calibration...\n");

  // ─── 1. Detect Hardware ─────────────────────────────────────────────
  NovaHardwareProfile hw = nova_detect_hardware();

  // ─── 2. CPU Calibration ─────────────────────────────────────────────
  int m, k;
  nova_get_optimal_block_size(&hw, &m, &k);

  NovaCPUCalibration *cpu_cal = &g_nova_sovereign_config.cpu;
  cpu_cal->matmul_tile_m = m;
  cpu_cal->matmul_tile_n = 4; // SIMD width (NEON=4, AVX=8)
  cpu_cal->matmul_tile_k = k;
  cpu_cal->matmul_prefetch_distance = 4;
  cpu_cal->attn_tile_size = 32;

  // Thread strategy
  cpu_cal->worker_threads = hw.cpu.core_count;
  cpu_cal->use_smt = (hw.cpu.threads_per_core > 1);

  // big.LITTLE: prefer P-cores for compute
  if (hw.cpu.perf_core_count > 0) {
    cpu_cal->pin_perf_cores = true;
    cpu_cal->worker_threads = hw.cpu.perf_core_count;
  }

  // Adjust tile/SIMD based on ISA
  if (hw.cpu.has_avx512 || hw.cpu.has_sve) {
    cpu_cal->matmul_tile_n = 16; // 512-bit / 32-bit = 16 floats
  } else if (hw.cpu.has_avx2) {
    cpu_cal->matmul_tile_n = 8; // 256-bit / 32-bit = 8 floats
  } else if (hw.cpu.has_neon) {
    cpu_cal->matmul_tile_n = 4; // 128-bit / 32-bit = 4 floats
  }

  // Placeholder benchmarks (real microbenchmarks will fill these)
  cpu_cal->measured_gflops = 0.0;
  cpu_cal->measured_bandwidth_gb_s = 0.0;

  // ─── 3. GPU Calibration ─────────────────────────────────────────────
  g_nova_sovereign_config.gpu_count = hw.gpu_count;
  g_nova_sovereign_config.preferred_gpu_index = -1; // CPU by default

  for (int i = 0; i < hw.gpu_count; i++) {
    NovaGPUCalibration *gpu_cal = &g_nova_sovereign_config.gpus[i];
    gpu_cal->backend = hw.gpus[i].backend;
    gpu_cal->calibrated = false;

    if (!hw.gpus[i].available)
      continue;

    // Default threadgroup / block config per backend
    switch (hw.gpus[i].backend) {
    case NOVA_GPU_METAL:
      gpu_cal->threadgroup_x = 16;
      gpu_cal->threadgroup_y = 16;
      gpu_cal->block_size_1d = 256;
      gpu_cal->gpu_tile_m = 32;
      gpu_cal->gpu_tile_n = 32;
      gpu_cal->gpu_tile_k = 32;
      gpu_cal->use_shared_memory = true;
      gpu_cal->use_fp16_accumulate = true; // Apple supports FP16 natively
      gpu_cal->calibrated = true;
      g_nova_sovereign_config.preferred_gpu_index = i;
      break;

    case NOVA_GPU_CUDA:
      gpu_cal->threadgroup_x = 16;
      gpu_cal->threadgroup_y = 16;
      gpu_cal->block_size_1d = 256;
      gpu_cal->gpu_tile_m = 64;
      gpu_cal->gpu_tile_n = 64;
      gpu_cal->gpu_tile_k = 16;
      gpu_cal->use_shared_memory = true;
      gpu_cal->use_fp16_accumulate = true; // Tensor Cores
      gpu_cal->calibrated = true;
      g_nova_sovereign_config.preferred_gpu_index = i;
      break;

    case NOVA_GPU_VULKAN:
      gpu_cal->threadgroup_x = 8;
      gpu_cal->threadgroup_y = 8;
      gpu_cal->block_size_1d = 128;
      gpu_cal->gpu_tile_m = 16;
      gpu_cal->gpu_tile_n = 16;
      gpu_cal->gpu_tile_k = 16;
      gpu_cal->use_shared_memory = true;
      gpu_cal->use_fp16_accumulate = false;
      // NOT calibrated until runtime Vulkan probe
      break;

    case NOVA_GPU_DIRECTML:
      gpu_cal->threadgroup_x = 16;
      gpu_cal->threadgroup_y = 16;
      gpu_cal->block_size_1d = 256;
      // NOT calibrated until DX12 enumeration
      break;

    default:
      break;
    }
  }

  // ─── 4. Finalize ────────────────────────────────────────────────────
  g_nova_sovereign_config.is_calibrated = true;
  g_nova_sovereign_config.calibration_timestamp = (uint64_t)time(NULL);
  g_nova_sovereign_config.target_os = hw.cpu.os;
  g_nova_sovereign_config.target_arch = hw.cpu.arch;

  // ─── 5. Report ──────────────────────────────────────────────────────
  const char *gpu_pref = "CPU (no GPU preferred)";
  if (g_nova_sovereign_config.preferred_gpu_index >= 0) {
    gpu_pref = hw.gpus[g_nova_sovereign_config.preferred_gpu_index].name;
  }

  printf("✅ [SOVEREIGN] Calibration Complete.\n");
  printf("   CPU Tile: %dx%dx%d | Workers: %d%s\n", cpu_cal->matmul_tile_m,
         cpu_cal->matmul_tile_k, cpu_cal->matmul_tile_n,
         cpu_cal->worker_threads,
         cpu_cal->pin_perf_cores ? " (P-cores pinned)" : "");
  printf("   GPU Preferred: %s\n", gpu_pref);
  printf("   GPU Backends: %d available\n\n", hw.gpu_count);
}
