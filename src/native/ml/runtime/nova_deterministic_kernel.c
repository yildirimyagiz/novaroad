#include "../../include/nova_deterministic_kernel.h"
#include "formal/nova_formal.h"
#include <fenv.h>
#include <math.h>
#include <stdio.h>

#ifdef __x86_64__
#include <pmmintrin.h>
#include <xmmintrin.h>
#endif

static DeterministicKernelConfig g_config;
static uint32_t g_logical_clock = 0;

void nova_kernel_init(DeterministicKernelConfig config) {
  g_config = config;

  if (config.strict_fp) {
#ifdef __x86_64__
    // Enable Flush-to-Zero and Denormals-are-Zero for deterministic perf and
    // consistency
    _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
    // Set standard rounding mode
    fesetround(FE_TONEAREST);
  }

  printf("🛡️ Nova Deterministic Kernel Active (Strict FP: %s, Seed: %u)\n",
         config.strict_fp ? "ON" : "OFF", config.seed);
}

double nova_fp_stabilize(double value) {
  if (!g_config.strict_fp)
    return value;

  // Check for NaN and canonicalize
  if (isnan(value))
    return NAN;

  // Check for subnormals (if FTZ is not hardware supported)
  if (fpclassify(value) == FP_SUBNORMAL)
    return 0.0;

  // Canonicalize -0.0 to 0.0 for hash consistency if needed
  if (value == 0.0)
    return 0.0;

  return value;
}

float nova_fp_stabilize_f(float value) {
  if (!g_config.strict_fp)
    return value;
  if (isnan(value))
    return NAN;
  if (fpclassify(value) == FP_SUBNORMAL)
    return 0.0f;
  if (value == 0.0f)
    return 0.0f;
  return value;
}

void nova_kernel_yield(void) {
  // Increment logical clock on every return to maintain relative order
  g_logical_clock++;
}

uint32_t nova_kernel_get_logical_time(void) { return g_logical_clock; }

void nova_kernel_enforce_invariants(void) {
  // Formal Execution Snapshot (GÖDEL)
  if (!nova_formal_check_invariant("runtime_drift_detection", NULL)) {
    fprintf(stderr, "⚠️ [Gödel] Runtime Veto: Execution drift detected\n");
  }
}
