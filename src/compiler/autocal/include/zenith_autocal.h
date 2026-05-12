/**
 * 🎯 zenith_autocal.h - Nova Auto-calibration Engine
 *
 * Part of the "GPU-Army" V10 strategy. This engine measures the local
 * hardware performance (CPU SIMD vs Metal GPU) and determines the optimal
 * thresholds for the 4-Layer Army (4LUA) system.
 */

#ifndef ZENITH_AUTOCAL_H
#define ZENITH_AUTOCAL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    // Thresholds in elements (size = M * N)
    uint64_t l1_reflex_threshold; // Max size for Silicon Reflex (L1)
    uint64_t l2_daemon_threshold; // Max size for Kernel Daemon (L2)
    uint64_t l3_web_threshold;    // Max size for Web Nexus (L3)

    // Performance metrics
    double cpu_gflops;
    double gpu_gflops;
    double p2p_bandwidth_mbps;
} NovaAutocalConfig;

/**
 * Global configuration instance.
 */
extern NovaAutocalConfig g_nova_autocal_config;

/**
 * Initialize and run the auto-calibration sequence (original, simple).
 * This runs benchmarks across all available tiers.
 */
void zenith_autocal_run(void);

/**
 * Run comprehensive calibration with all benchmarks.
 */
void zenith_autocal_run_comprehensive(void);

/**
 * Run quick calibration (subset of benchmarks).
 */
void zenith_autocal_run_quick(void);

/**
 * Save the calibration data to a persistent config file.
 */
bool zenith_autocal_save(const char *path);

/**
 * Load calibration data from a persistent config file.
 */
bool zenith_autocal_load(const char *path);

/**
 * Reset calibration to default "Ultra-Safe" values.
 */
void zenith_autocal_reset_defaults(void);

#endif // ZENITH_AUTOCAL_H
