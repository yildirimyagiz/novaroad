/**
 * src/tools/nova_synthetic_tensor_demo.c
 * NOVA vs MOJO Strategy: Synthetic Tensor Generation (Compute-over-Memory)
 */

#include "nova_decision_system.h"
#include "nova_kernels.h"
#include "nova_tensor.h"
#include <mach/mach_time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t now_ns() {
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
}

#define SEQ_LEN 4096
#define DIM 128

// --- Scenario 1: Materialized ---
// Positional Encodings are loaded from memory (HBM/DRAM)
void run_materialized_pe(const float *pe_memory, float *output) {
  for (int i = 0; i < SEQ_LEN * DIM; i++) {
    output[i] += pe_memory[i]; // Simulated memory load + addition
  }
}

void run_synthetic_pe(NovaTensor *pe_tensor) {
  nova_kernel_synthetic_pe_f32(pe_tensor);
}

// --- Scenario 3: Recurrent (Nova Elite Path) ---
// Generated via FMA recurrence (sin(x+d) = ...)
void run_recurrent_pe(NovaTensor *pe_tensor) {
  nova_kernel_recurrent_pe_f32(pe_tensor);
}

int main() {
  printf("🎬 NOVA HYPER-SYNTHETICS: Materialized vs Vector vs Recurrence\n");
  printf("   Workload: Sinusoidal Positional Encoding (Seq=%d, Dim=%d)\n\n",
         SEQ_LEN, DIM);

  int64_t shape[] = {SEQ_LEN, DIM};
  NovaTensor *pe_tensor =
      nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
  float *pe_memory = malloc(SEQ_LEN * DIM * sizeof(float));
  float *output = malloc(SEQ_LEN * DIM * sizeof(float));
  for (int i = 0; i < SEQ_LEN * DIM; i++)
    pe_memory[i] = (float)i;

  // 1. Materialized Bench
  uint64_t t0 = now_ns();
  int iters = 1000;
  for (int n = 0; n < iters; n++)
    run_materialized_pe(pe_memory, output);
  uint64_t t1 = now_ns();
  double mat_ms = (double)(t1 - t0) / (iters * 1e6);
  printf("💎 [MATERIALIZED] Latency: %.4f ms (Bandwidth Bound)\n", mat_ms);

  // 2. Synthetic Bench (Vectorized)
  uint64_t t2 = now_ns();
  for (int n = 0; n < iters; n++)
    run_synthetic_pe(pe_tensor);
  uint64_t t3 = now_ns();
  double syn_ms = (double)(t3 - t2) / (iters * 1e6);
  printf("💎 [SYNTHETIC]    Latency: %.4f ms (Compute Bound | HYPER-NEON)\n",
         syn_ms);

  // 3. Recurrent Bench (Nova Sweet Spot)
  uint64_t t4 = now_ns();
  for (int n = 0; n < iters; n++)
    run_recurrent_pe(pe_tensor);
  uint64_t t5 = now_ns();
  double rec_ms = (double)(t5 - t4) / (iters * 1e6);
  printf("💎 [RECURRENT]    Latency: %.4f ms (FMA Sweet Spot | 0-Bandwidth)\n",
         rec_ms);

  printf("\n🔥 IMPACT: Synthesis avoids moving %.2f MB of PE data per "
         "inference step.\n",
         (float)SEQ_LEN * DIM * sizeof(float) / 1e6);

  if (syn_ms < mat_ms) {
    printf("✅ VERDICT: On M2 Max, computing PE on-fly is FASTER than loading "
           "from memory.\n");
  } else {
    printf("⚠️  NOTICE: ALU cost is higher, but memory bandwidth is PRESERVED "
           "for weights.\n");
  }

  free(pe_memory);
  free(output);
  return 0;
}
