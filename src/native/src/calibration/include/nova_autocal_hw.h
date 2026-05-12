// 🦅 Nova Sovereign Calibration — Universal Hardware Detection
// All Platforms: macOS, Linux, Windows, iOS, Android
// All Compute: CPU (x86/ARM), GPU (Metal/CUDA/Vulkan/OpenCL)

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// TARGET OS DETECTION
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  NOVA_OS_UNKNOWN = 0,
  NOVA_OS_MACOS,
  NOVA_OS_LINUX,
  NOVA_OS_WINDOWS,
  NOVA_OS_IOS,
  NOVA_OS_ANDROID,
  NOVA_OS_FREEBSD,
  NOVA_OS_WASM,
} NovaTargetOS;

// ═══════════════════════════════════════════════════════════════════════════
// CPU ARCHITECTURE
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  NOVA_ARCH_UNKNOWN = 0,
  NOVA_ARCH_X86_64,
  NOVA_ARCH_ARM64, // Apple Silicon, Snapdragon, Graviton
  NOVA_ARCH_ARM32, // Older Android / Embedded
  NOVA_ARCH_RISCV64,
  NOVA_ARCH_WASM32,
} NovaArchType;

// ═══════════════════════════════════════════════════════════════════════════
// GPU BACKEND TYPE
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  NOVA_GPU_NONE = 0,
  NOVA_GPU_METAL,    // Apple (macOS, iOS, iPadOS)
  NOVA_GPU_CUDA,     // NVIDIA
  NOVA_GPU_VULKAN,   // Cross-platform (AMD, NVIDIA, Intel, Qualcomm)
  NOVA_GPU_OPENCL,   // Legacy cross-platform
  NOVA_GPU_ROCM,     // AMD HIP/ROCm
  NOVA_GPU_WEBGPU,   // Browser
  NOVA_GPU_DIRECTML, // Windows (DirectX 12)
} NovaGPUBackend;

// ═══════════════════════════════════════════════════════════════════════════
// CPU HARDWARE PROFILE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char name[256];
  NovaTargetOS os;
  NovaArchType arch;
  int core_count;
  int perf_core_count; // Apple big.LITTLE / ARM DynamIQ
  int efficiency_core_count;
  int threads_per_core; // SMT / Hyper-Threading

  // Cache Hierarchy (bytes)
  size_t l1_cache_size;
  size_t l2_cache_size;
  size_t l3_cache_size;

  // SIMD / Matrix Extensions
  bool has_neon;        // ARM NEON (ARM64)
  bool has_sve;         // ARM SVE  (Graviton3+)
  bool has_sve2;        // ARM SVE2 (Graviton4+)
  bool has_amx;         // Apple Matrix Extension (M1+)
  bool has_dotprod;     // ARM Dot Product (int8 acceleration)
  bool has_fp16;        // Native FP16 support
  bool has_bf16;        // BFloat16 support (Graviton3, Intel AMX)
  bool has_i8mm;        // Int8 Matrix Multiply (ARMv8.6+)
  bool has_sse42;       // x86 SSE4.2
  bool has_avx2;        // x86 AVX2
  bool has_avx512;      // x86 AVX-512
  bool has_avx512_vnni; // x86 AVX-512 VNNI (int8 inference)
  bool has_intel_amx;   // Intel AMX (Sapphire Rapids+)

  // Frequency
  double base_freq_ghz;
  double max_freq_ghz;

  // Memory
  size_t total_ram_bytes;
} NovaCPUProfile;

// ═══════════════════════════════════════════════════════════════════════════
// GPU HARDWARE PROFILE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char name[256];
  NovaGPUBackend backend;
  bool available;

  // Compute
  int compute_units;         // CUDA Cores / Stream Processors / GPU Cores
  int max_threads_per_group; // Metal threadgroup / CUDA block
  int max_threadgroup_x;
  int max_threadgroup_y;
  int max_threadgroup_z;
  double peak_tflops_fp32;
  double peak_tflops_fp16;

  // Memory
  size_t vram_bytes;
  double memory_bandwidth_gb_s;
  int memory_bus_width; // bits
} NovaGPUProfile;

// ═══════════════════════════════════════════════════════════════════════════
// COMBINED HARDWARE PROFILE (Full Machine Fingerprint)
// ═══════════════════════════════════════════════════════════════════════════

#define NOVA_MAX_GPUS 8

typedef struct {
  NovaCPUProfile cpu;
  NovaGPUProfile gpus[NOVA_MAX_GPUS];
  int gpu_count;
} NovaHardwareProfile;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

/// Detect full machine hardware (CPU + all GPUs).
NovaHardwareProfile nova_detect_hardware(void);

/// Returns optimal tile sizes for MatMul based on L1/L2 cache.
void nova_get_optimal_block_size(const NovaHardwareProfile *profile, int *m,
                                 int *k);

/// Print full hardware profile to stdout.
void nova_print_hardware_profile(const NovaHardwareProfile *profile);
