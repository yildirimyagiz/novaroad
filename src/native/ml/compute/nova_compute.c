#include "../../include/nova_compute.h"
#include "../../include/nova_adaptive_optimizer.h"
#include "../../include/nova_dispatcher.h"
#include "../../include/nova_execution_fabric.h"
#include "../../include/nova_profiler_v2.h"
#include <stdio.h>

void nova_compute_init(NovaContext *ctx) {
  printf("🚀 Nova Compute ABI Layer Initializing...\n");
}

void nova_compute_shutdown(NovaContext *ctx) {
  printf("🛑 Nova Compute ABI Layer Shutting Down...\n");
}

void nova_compute_matmul(NovaContext *ctx, const NovaComputeTensor *A,
                           const NovaComputeTensor *B,
                           NovaComputeTensor *C) {
  // 1. Profiling - Begin
  nova_profiler_begin(ctx->profiler, "nova_compute_matmul");

  // 2. Optimization - Select Strategy
  ExecutionStrategy strategy =
      nova_adaptive_select_strategy(ctx->optimizer, "matmul_op");

  // 3. Scheduling - Prepare Task
  // Map Strategy to Backend
  BackendType preferred = BACKEND_CPU;
  if (strategy == STRATEGY_SIMD)
    preferred = BACKEND_SIMD_AVX512;
  if (strategy == STRATEGY_GPU)
    preferred = BACKEND_METAL_GPU;

  // 4. Dispatcher - Route to Fabric
  DispatchTask task = {
      .kernel = NULL, // In a real system, this would be the matmul kernel
      .args = (void *)A,
      .target_backend = preferred};
  bool success = nova_dispatcher_route(ctx, task);

  // 5. Profiling - End & Feedback
  nova_profiler_end(ctx->profiler, "nova_compute_matmul");

  if (!success) {
    printf("⚠️ Matmul dispatch failed for preferred backend, using fallback.\n");
  }
}

void nova_compute_tensor_add(NovaContext *ctx, const NovaComputeTensor *A,
                               const NovaComputeTensor *B,
                               NovaComputeTensor *C) {
  // Similar flow as matmul but for addition
}

void nova_compute_dispatch_custom(NovaContext *ctx, void (*kernel)(void *),
                                    void *args) {
  // Generic kernel dispatch through the fabric
}
