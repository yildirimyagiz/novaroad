/**
 * 🦅 nova_army_platform.h - GPU-Army V10 Platform Abstraction Layer
 *
 * This header provides a unified interface for the GPU-Army to dispatch
 * compute operations across all target platforms:
 *
 *   ┌────────────────┬──────────────┬───────────────┬───────────────┐
 *   │   Platform     │  L1 (SIMD)   │  L1 (GPU)     │  L2 (Thread)  │
 *   ├────────────────┼──────────────┼───────────────┼───────────────┤
 *   │ macOS/iPhone   │ NEON FP16    │ Metal         │ GCD           │
 *   │ Android        │ NEON FP16    │ Vulkan        │ pthread       │
 *   │ Windows        │ AVX2/AVX-512 │ DirectX 12    │ Win32 Thread  │
 *   │ Linux (NVIDIA) │ AVX2         │ CUDA          │ pthread       │
 *   │ Linux (AMD)    │ AVX2         │ ROCm/Vulkan   │ pthread       │
 *   │ WASM           │ WASM SIMD    │ WebGPU        │ Web Workers   │
 *   └────────────────┴──────────────┴───────────────┴───────────────┘
 */

#ifndef NOVA_ARMY_PLATFORM_H
#define NOVA_ARMY_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// PLATFORM DETECTION
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    ARMY_PLATFORM_APPLE_SILICON, // macOS + iOS (ARM64 + Metal)
    ARMY_PLATFORM_ANDROID_ARM,   // Android (ARM64 + Vulkan)
    ARMY_PLATFORM_WINDOWS_X86,   // Windows (x86_64 + DirectX/Vulkan)
    ARMY_PLATFORM_LINUX_NVIDIA,  // Linux + NVIDIA (x86_64 + CUDA)
    ARMY_PLATFORM_LINUX_AMD,     // Linux + AMD (x86_64 + ROCm)
    ARMY_PLATFORM_LINUX_GENERIC, // Linux generic (x86_64 + Vulkan)
    ARMY_PLATFORM_WASM,          // WebAssembly (WASM SIMD + WebGPU)
    ARMY_PLATFORM_UNKNOWN
} ArmyPlatformType;

typedef enum {
    ARMY_SIMD_NEON,      // ARM NEON (128-bit)
    ARMY_SIMD_NEON_FP16, // ARM NEON + FP16 (Apple, Cortex-A76+)
    ARMY_SIMD_AVX2,      // x86 AVX2 (256-bit)
    ARMY_SIMD_AVX512,    // x86 AVX-512 (512-bit)
    ARMY_SIMD_WASM128,   // WASM SIMD (128-bit)
    ARMY_SIMD_NONE
} ArmySIMDType;

typedef enum {
    ARMY_GPU_METAL,  // Apple Metal
    ARMY_GPU_VULKAN, // Vulkan (Android, Linux, Windows)
    ARMY_GPU_CUDA,   // NVIDIA CUDA
    ARMY_GPU_ROCM,   // AMD ROCm
    ARMY_GPU_DX12,   // Microsoft DirectX 12
    ARMY_GPU_WEBGPU, // WebGPU (Browser)
    ARMY_GPU_NONE
} ArmyGPUType;

typedef enum {
    ARMY_THREAD_GCD,      // Apple Grand Central Dispatch
    ARMY_THREAD_PTHREAD,  // POSIX threads
    ARMY_THREAD_WIN32,    // Windows Thread Pool
    ARMY_THREAD_WEBWORKER // Web Workers (WASM)
} ArmyThreadType;

// ═══════════════════════════════════════════════════════════════════════════
// PLATFORM CAPABILITIES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    ArmyPlatformType platform;
    ArmySIMDType simd;
    ArmyGPUType gpu;
    ArmyThreadType threading;

    // Hardware details
    int core_count;
    int simd_width_bits; // 128, 256, 512
    uint64_t gpu_memory_bytes;
    bool has_fp16;
    bool has_unified_memory; // Apple Silicon, some mobile
    char device_name[128];
} ArmyPlatformCaps;

// ═══════════════════════════════════════════════════════════════════════════
// PLATFORM DISPATCH API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Detect the current platform and populate capabilities.
 */
ArmyPlatformCaps nova_army_detect_platform(void);

/**
 * Get a human-readable name for the platform.
 */
const char *nova_army_platform_name(ArmyPlatformType platform);

/**
 * L1 SIMD Dispatch: Execute SIMD-optimized kernel on the detected platform.
 * Returns 0 on success, -1 if no SIMD available.
 */
int64_t nova_army_simd_matmul(const ArmyPlatformCaps *caps, const float *mat_a, const float *mat_b,
                              float *mat_c, int64_t rows, int64_t cols, int64_t inner);

int64_t nova_army_simd_flash_attention(const ArmyPlatformCaps *caps, const float *query,
                                       const float *key, const float *value, float *output,
                                       int seq_len, int head_dim);

/**
 * L1 GPU Dispatch: Execute GPU compute kernel on the detected platform.
 * Returns 0 on success, -1 if no GPU available.
 */
int64_t nova_army_gpu_matmul(const ArmyPlatformCaps *caps, const float *mat_a, const float *mat_b,
                             float *mat_c, int64_t rows, int64_t cols, int64_t inner);

int64_t nova_army_gpu_flash_attention(const ArmyPlatformCaps *caps, const float *query,
                                      const float *key, const float *value, float *output,
                                      int seq_len, int head_dim);

/**
 * L2 Threaded Dispatch: Execute parallel kernel on the detected platform.
 * Returns 0 on success.
 */
int64_t nova_army_threaded_matmul(const ArmyPlatformCaps *caps, const float *mat_a,
                                  const float *mat_b, float *mat_c, int64_t rows, int64_t cols,
                                  int64_t inner);

/**
 * Print detailed platform information.
 */
void nova_army_print_platform(const ArmyPlatformCaps *caps);

#endif // NOVA_ARMY_PLATFORM_H
