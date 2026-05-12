#ifndef NOVA_FP16_KERNELS_H
#define NOVA_FP16_KERNELS_H

#include "../ml/nova_tensor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declaration if needed, but we don't include advanced_optimizations
// here to avoid circularity. Advanced optimizations should include us.

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * FP16/BF16 Optimized Kernels for Nova ML
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifdef __APPLE__
typedef struct {
  void *metal_device;
  void *metal_library;
  void *metal_pipeline;
} MetalKernelContext;

int nova_fp16_matmul_metal(const float *a, const float *b, float *c,
                             int64_t m, int64_t n, int64_t k);
int nova_fp16_conv2d_metal(const float *input, const float *weight,
                             const float *bias, float *output, int in_h,
                             int in_w, int out_h, int out_w, int in_c,
                             int out_c, int k_h, int k_w, int stride,
                             int padding);
int nova_fp16_attention_metal(const __fp16 *q, const __fp16 *k,
                                const __fp16 *v, __fp16 *output, int seq_len,
                                int embed_dim, int num_heads);
#endif

typedef enum {
  NOVA_ACCUM_FP32,
  NOVA_ACCUM_FP16,
  NOVA_ACCUM_BF16,
} NovaAccumulationMode;

typedef struct {
  NovaAccumulationMode accumulation_mode;
  bool use_tensor_cores;
  bool use_warp_specialization;
  int max_threads_per_block;
  int shared_memory_size_kb;
  bool enable_profiling;
} NovaNumericsPolicy;

typedef enum {
  NOVA_SD_SUCCESS = 0,
  NOVA_SD_ERROR_MEMORY_ALLOCATION = -1,
  NOVA_SD_ERROR_MODEL_LOAD = -2,
  NOVA_SD_ERROR_TENSOR_SHAPE_MISMATCH = -3,
  NOVA_SD_ERROR_NUMERICAL_ERROR = -4,
  NOVA_SD_ERROR_BACKEND_COMPUTE = -5,
  NOVA_SD_ERROR_INVALID_INPUT = -6,
  NOVA_SD_ERROR_TOKENIZER = -7,
  NOVA_SD_ERROR_SCHEDULER = -8,
  NOVA_SD_ERROR_FILE_IO = -9,
  NOVA_SD_ERROR_UNSUPPORTED_OPERATION = -10,
} NovaSDErrorCode;

typedef struct {
  NovaSDErrorCode code;
  const char *message;
  const char *file;
  int line;
} NovaSDError;

typedef struct NovaComputeContext {
  NovaBackendType backend;
  void *context;
  NovaNumericsPolicy numerics_policy;
  int fp16_enabled;
  int bf16_enabled;
} NovaComputeContext;

NovaComputeContext *nova_compute_init(NovaBackendType preferred_backend);
void nova_compute_free(NovaComputeContext *ctx);

int nova_matmul(NovaComputeContext *ctx, const void *a, const void *b,
                  void *c, int64_t m, int64_t n, int64_t k, int use_fp16);
int nova_conv2d(NovaComputeContext *ctx, const void *input,
                  const void *weight, const void *bias, void *output, int in_h,
                  int in_w, int out_h, int out_w, int in_c, int out_c, int k_h,
                  int k_w, int stride, int padding, int use_fp16);
int nova_attention(NovaComputeContext *ctx, const void *q, const void *k,
                     const void *v, void *output, int seq_len, int embed_dim,
                     int num_heads, int use_fp16);

#endif // NOVA_FP16_KERNELS_H
