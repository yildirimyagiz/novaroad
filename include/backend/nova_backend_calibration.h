/**
 * @file nova_backend_calibration.h
 * @brief Auto-calibration API for backend selection
 */

#ifndef NOVA_BACKEND_CALIBRATION_H
#define NOVA_BACKEND_CALIBRATION_H

#include "nova_backend_dispatch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run auto-calibration to determine optimal backends for different workload sizes.
 * This runs micro-benchmarks and may take several seconds.
 */
void nova_backend_calibrate(void);

/**
 * Get recommended backend for a matmul operation based on calibration data.
 * Returns NOVA_BACKEND_AUTO if not calibrated.
 */
NovaBackendType nova_backend_recommend_for_matmul(int64_t m, int64_t n, int64_t k);

/**
 * Check if calibration has been performed.
 */
int nova_backend_is_calibrated(void);

/**
 * Save calibration data to file for future use.
 */
void nova_backend_save_calibration(const char *filename);

/**
 * Load previously saved calibration data.
 */
void nova_backend_load_calibration(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_BACKEND_CALIBRATION_H */
