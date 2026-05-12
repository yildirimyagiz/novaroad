// 🦅 Nova ign Calibration — Reporting Engine
// Full platform report: CPU + GPU + strategies.

#include "../../include/nova_autocal_report.h"
#include "../../include/nova_autocal_schema.h"
#include <stdio.h>

static const char *gpu_backend_name(NovaGPUBackend b) {
  switch (b) {
  case NOVA_GPU_METAL:
    return "Metal";
  case NOVA_GPU_CUDA:
    return "CUDA";
  case NOVA_GPU_VULKAN:
    return "Vulkan";
  case NOVA_GPU_OPENCL:
    return "OpenCL";
  case NOVA_GPU_ROCM:
    return "ROCm";
  case NOVA_GPU_WEBGPU:
    return "WebGPU";
  case NOVA_GPU_DIRECTML:
    return "DirectML";
  default:
    return "None";
  }
}

static const char *os_name(NovaTargetOS os) {
  switch (os) {
  case NOVA_OS_MACOS:
    return "macOS";
  case NOVA_OS_LINUX:
    return "Linux";
  case NOVA_OS_WINDOWS:
    return "Windows";
  case NOVA_OS_IOS:
    return "iOS";
  case NOVA_OS_ANDROID:
    return "Android";
  case NOVA_OS_FREEBSD:
    return "FreeBSD";
  case NOVA_OS_WASM:
    return "WebAssembly";
  default:
    return "Unknown";
  }
}

static const char *arch_name(NovaArchType a) {
  switch (a) {
  case NOVA_ARCH_X86_64:
    return "x86_64";
  case NOVA_ARCH_ARM64:
    return "ARM64";
  case NOVA_ARCH_ARM32:
    return "ARM32";
  case NOVA_ARCH_RISCV64:
    return "RISC-V 64";
  case NOVA_ARCH_WASM32:
    return "WASM32";
  default:
    return "Unknown";
  }
}

void nova_autocal_report(void) {
  if (!g_nova_sovereign_config.is_calibrated) {
    printf("⚠️  [SOVEREIGN] System not yet calibrated.\n");
    return;
  }

  const NovaCalibrationSchema *s = &g_nova_sovereign_config;
  const NovaCPUCalibration *cpu = &s->cpu;

  printf("\n");
  printf("╔═══════════════════════════════════════════════════════════╗\n");
  printf("║  🦅  Eagle-Eye Calibration Report — Sovereign v8.0       ║\n");
  printf("╠═══════════════════════════════════════════════════════════╣\n");
  printf("║  Platform: %-10s | Arch: %-8s                     ║\n",
         os_name(s->target_os), arch_name(s->target_arch));
  printf("╠═══════════════════════════════════════════════════════════╣\n");
  printf("║  CPU STRATEGY                                            ║\n");
  printf("║  Tile: [M=%2d K=%3d N=%2d] Prefetch: %d                   ║\n",
         cpu->matmul_tile_m, cpu->matmul_tile_k, cpu->matmul_tile_n,
         cpu->matmul_prefetch_distance);
  printf("║  Workers: %2d  SMT: %s  P-Core Pin: %s               ║\n",
         cpu->worker_threads, cpu->use_smt ? "ON " : "OFF",
         cpu->pin_perf_cores ? "ON " : "OFF");
  printf("║  Attention Tile: %3d                                     ║\n",
         cpu->attn_tile_size);

  if (s->gpu_count > 0) {
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  GPU BACKENDS (%d detected)                               ║\n",
           s->gpu_count);

    for (int i = 0; i < s->gpu_count; i++) {
      const NovaGPUCalibration *g = &s->gpus[i];
      const char *pref = (i == s->preferred_gpu_index) ? "★" : " ";
      const char *cal = g->calibrated ? "✓" : "…";
      printf("║  %s [%s] %-8s  TG:%dx%d  Tile:%dx%dx%d  FP16:%s %s   ║\n", pref,
             cal, gpu_backend_name(g->backend), g->threadgroup_x,
             g->threadgroup_y, g->gpu_tile_m, g->gpu_tile_n, g->gpu_tile_k,
             g->use_fp16_accumulate ? "Y" : "N",
             g->use_shared_memory ? "SHMEM" : "");
    }
  }

  printf("╚═══════════════════════════════════════════════════════════╝\n\n");
}
