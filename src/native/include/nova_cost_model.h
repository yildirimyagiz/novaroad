/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_cost_model.h — Graph Cost Estimation Engine
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Provides theoretical cost estimation (FLOPs, Bandwidth, Latency) for
 * Nova Compute Graphs to guide the optimizer logic (Fusion, Tiling).
 */

#ifndef NOVA_COST_MODEL_H
#define NOVA_COST_MODEL_H

#include "nova_graph_structs.h"
#include "nova_tensor.h"
#include <stdbool.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * COST METRICS
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  /* Compute Costs */
  double total_flops; // Total Floating Point Operations
  double total_iops;  // Total Integer Operations (Indexing etc.)

  /* Memory Costs */
  double memory_read_bytes;  // Total bytes read from global memory
  double memory_write_bytes; // Total bytes written to global memory
  double cache_miss_rate;    // Estimated cache miss rate (0.0 - 1.0)

  /* Derived Metrics */
  double arithmetic_intensity; // FLOPs / Byte
  double estimated_latency_us; // Estimated runtime in microseconds

  /* Bottleneck Analysis */
  bool is_memory_bound;   // True if arithmetic_intensity < ridge_point
  bool is_compute_bound;  // True otherwise
  float saturation_score; // 0.0 - 1.0, how much of the HW is utilized
} NovaCostMetrics;

/* ═══════════════════════════════════════════════════════════════════════════
 * HARDWARE PROFILE
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  const char *device_name;

  /* Compute Capabilities */
  double peak_gflops_fp32;
  double peak_gflops_fp16;
  double peak_gflops_int8;

  /* Memory Hierarchy */
  double bandwidth_gbps; // Global Memory Bandwidth
  size_t l1_cache_size;  // Bytes
  size_t l2_cache_size;  // Bytes
  size_t l3_cache_size;  // Bytes (0 if N/A)

  /* Architectural details */
  int vector_width_bytes; // SIMD width (e.g., 32 for AVX2, 64 for AVX-512)
  double overhead_per_kernel_us; // Launch overhead
} NovaHardwareProfile;

/* ═══════════════════════════════════════════════════════════════════════════
 * API
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Initialize a default hardware profile for the target device type.
 */
void nova_cost_model_init_hw(NovaHardwareProfile *hw, NovaDevice device);

/**
 * Estimate cost for a single node in isolation.
 */
NovaCostMetrics
nova_cost_model_estimate_node(const NovaIRNode *node,
                                const NovaHardwareProfile *hw);

/**
 * Estimate cost for the entire graph, considering producer-consumer locality.
 * (Future: will simulate cache reuse between nodes)
 */
NovaCostMetrics
nova_cost_model_estimate_graph(const NovaIRGraph *graph,
                                 const NovaHardwareProfile *hw);

/**
 * Print a detailed cost report to stdout.
 */
void nova_cost_model_print_report(const NovaCostMetrics *metrics,
                                    const char *label);

#endif // NOVA_COST_MODEL_H
