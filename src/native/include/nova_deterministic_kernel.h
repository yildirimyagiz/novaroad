#ifndef NOVA_DETERMINISTIC_KERNEL_H
#define NOVA_DETERMINISTIC_KERNEL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA DETERMINISTIC EXECUTION KERNEL
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Ensures cross-device identical outputs and floating point stabilization.
 */

typedef struct {
  uint32_t seed;
  bool strict_fp;     // Force IEEE 754 strict mode
  bool no_reordering; // Prevent instruction reordering that affects precision
  bool thread_affinity;
} DeterministicKernelConfig;

// Core API
void nova_kernel_init(DeterministicKernelConfig config);

// Floating Point Stabilization
double nova_fp_stabilize(double value);
float nova_fp_stabilize_f(float value);

// Reproducible Scheduling
void nova_kernel_yield(void);
uint32_t nova_kernel_get_logical_time(void);

// Invariant Enforcement
void nova_kernel_enforce_invariants(void);

#endif // NOVA_DETERMINISTIC_KERNEL_H
