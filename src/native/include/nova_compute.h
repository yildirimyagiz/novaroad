#ifndef NOVA_COMPUTE_H
#define NOVA_COMPUTE_H

#include "nova_context.h"
#include <stddef.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA COMPUTE ABI - Backend Agnostic Execution
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * The public contract for all high-performance operations.
 */

typedef struct {
  float *data;
  size_t rows;
  size_t cols;
  uint32_t flags;
} NovaComputeTensor;

// Lifecycle
void nova_compute_init(NovaContext *ctx);
void nova_compute_shutdown(NovaContext *ctx);

// Core Tensor Operations (Agnostic)
void nova_compute_matmul(NovaContext *ctx, const NovaComputeTensor *A,
                           const NovaComputeTensor *B,
                           NovaComputeTensor *C);
void nova_compute_tensor_add(NovaContext *ctx, const NovaComputeTensor *A,
                               const NovaComputeTensor *B,
                               NovaComputeTensor *C);

// Advanced Dispatch
void nova_compute_dispatch_custom(NovaContext *ctx, void (*kernel)(void *),
                                    void *args);

// Economic & Performance hints
void nova_compute_optimize_path(NovaContext *ctx, const char *path_id);

#endif // NOVA_COMPUTE_H
