/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_autocal.h - Platform-specific maximum performance calibration
 *
 * Her platform için özellik eşitliği değil, maksimum verim hedeflenir.
 * CPU: tile/prefix autotune; GPU: threadgroup / block size benchmark.
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_AUTOCAL_H
#define NOVA_AUTOCAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOVA_PLATFORM_CPU = 0,
    NOVA_PLATFORM_METAL,
    NOVA_PLATFORM_CUDA,
    NOVA_PLATFORM_ROCM,
    NOVA_PLATFORM_VULKAN,
    NOVA_PLATFORM_OPENCL,
    NOVA_PLATFORM_COUNT
} NovaPlatformId;

/** Per-platform best config (max throughput, not feature parity) */
typedef struct {
    NovaPlatformId platform;
    char device_name[128];

    /* CPU / shared */
    int matmul_tile_m;
    int matmul_tile_n;
    int matmul_tile_k;
    int prefetch_distance;
    int attn_tile_size;

    /* GPU: best threadgroup / block size for matmul */
    int gpu_threadgroup_x;
    int gpu_threadgroup_y;
    int gpu_block_size; /* CUDA/ROCm 1D block size for vector ops */

    double best_matmul_ns; /* best matmul time (ns) */
    double peak_gflops;    /* achieved GFLOPS */
    double memory_gb_s;    /* measured memory BW */
    bool calibrated;
} NovaAutocalPlatformConfig;

/** Run full calibration for current and available backends (max perf per platform) */
void nova_autocal_run(void);

/** Run calibration only for the given platform (e.g. after backend init) */
void nova_autocal_run_platform(NovaPlatformId platform);

/**
 * 🦅 Benchmark & Autotuning Utilities
 * Used for kernel-level performance measurement.
 */
typedef struct AutocalContext AutocalContext;

typedef struct {
    const char *name;
    void (*setup)(void *ctx);
    void (*execute)(void *ctx);
    void (*teardown)(void *ctx);
    void *context;
    int iterations;
} AutocalWorkload;

AutocalContext *autocal_create(void);
void autocal_destroy(AutocalContext *ctx);
double autocal_measure_time(AutocalWorkload workload);

/** Get the best config for a platform (after calibration) */
const NovaAutocalPlatformConfig *nova_autocal_get_config(NovaPlatformId platform);

/** Print calibration report (all calibrated platforms) */
void nova_autocal_report(void);

/** Hardware detection only (populates device name / capabilities) */
void nova_autocal_detect_hardware(void);

/** High-res timer (ms) for benchmarks */
double nova_autocal_now_ms(void);

/** Export calibrated configs to a baseline file (e.g. m1_default.json) for this platform */
void nova_autocal_export_baseline(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_AUTOCAL_H */
