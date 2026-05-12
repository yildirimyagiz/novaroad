#ifndef NOVA_ML_TENSOR_H
#define NOVA_ML_TENSOR_H


// Forward declaration
typedef struct NovaContext NovaContext;

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../backend/nova_backend_dispatch.h"


/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR - The Execution-Aware Compute Entity
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  NOVA_DTYPE_FP32,
  NOVA_DTYPE_FP16,
  NOVA_DTYPE_BF16,
  NOVA_DTYPE_INT8,
  NOVA_DTYPE_INT32
} NovaDType;

typedef enum {
  NOVA_LAYOUT_CONTIGUOUS,
  NOVA_LAYOUT_TILED_8x8,
  NOVA_LAYOUT_PACKED_4BIT,
  NOVA_LAYOUT_VIRTUAL_SYNTHETIC, // Data is procedurally generated
  NOVA_LAYOUT_RECURRENT,         // Data generated via recurrence (FMA-only)
  NOVA_LAYOUT_NCHW,
  NOVA_LAYOUT_NHWC
} NovaLayout;

// Use NovaBackendType from backend dispatcher
// Map to backend types for compatibility
#define NOVA_DEVICE_CPU          0
#define NOVA_DEVICE_SIMD         0
#define NOVA_DEVICE_METAL_GPU    2
#define NOVA_DEVICE_REMOTE_FABRIC 0

typedef int NovaDevice;

typedef enum {
  NOVA_ACTIVATION_RELU,
  NOVA_ACTIVATION_SILU,
  NOVA_ACTIVATION_GELU,
  NOVA_ACTIVATION_SIGMOID,
  NOVA_ACTIVATION_NONE
} NovaActivationType;

typedef enum {
  NOVA_NORM_GROUP_NORM,
  NOVA_NORM_LAYER_NORM,
  NOVA_NORM_BATCH_NORM,
  NOVA_NORM_NONE
} NovaNormType;

typedef enum {
  NOVA_CONV_MODE_CONV2D,
  NOVA_CONV_MODE_DEPTHWISE,
  NOVA_CONV_MODE_POINTWISE
} NovaConvMode;

typedef enum {
  NOVA_LIFETIME_PERSISTENT,
  NOVA_LIFETIME_GRAPH_TEMP,
  NOVA_LIFETIME_SCRATCH,
  NOVA_LIFETIME_GPU_STAGING,
  NOVA_LIFETIME_SESSION,
  NOVA_LIFETIME_STATIC,
  NOVA_LIFETIME_TRANSIENT
} NovaLifetimeClass;

typedef struct NovaTensor {
  int64_t *shape;
  int64_t *strides;
  union {
    int ndim;
    int rank; /* Alias for ndim — used by contract API */
  };
  size_t total_elements;
  NovaDType dtype;
  NovaLayout layout;
  NovaDevice device;
  NovaContext *context;
  void *data;
  bool is_view;
  bool is_deterministic;
  struct NovaTensor *grad;
  bool requires_grad;
  double last_op_latency;
  // Debug Instrumentation (Phase-5)
  const char *creation_site;
  const char *last_kernel_name;
  int last_backend_id;
  bool last_invariant_check;
} NovaTensor;

typedef struct {
  NovaTensor *base_weight;
  NovaTensor *lora_down;
  NovaTensor *lora_up;
  float alpha;
  int rank;
  float scale;
  char *name;
} NovaLoRALayer;

typedef struct {
  float learning_rate;
  float beta1;
  float beta2;
  float epsilon;
  float weight_decay;
  int step;
} NovaAdamW;

NovaTensor *nova_tensor_create(NovaContext *ctx, int64_t *shape, int ndim,
                                   NovaDType dtype);
NovaTensor *nova_tensor_create_on(NovaContext *ctx, int64_t *shape,
                                      int ndim, NovaDType dtype,
                                      NovaDevice device);
NovaTensor *nova_tensor_from_data(void *data, int64_t *shape, int ndim,
                                      int dtype, bool copy);
void nova_tensor_destroy(NovaTensor *t);

// Property accessors
void *nova_tensor_data(NovaTensor *t);
int64_t *nova_tensor_shape(NovaTensor *t);
int nova_tensor_ndim(NovaTensor *t);
size_t nova_tensor_size(NovaTensor *t);

// Operations (Eager)
NovaTensor *nova_tensor_add(NovaTensor *a, NovaTensor *b);
NovaTensor *nova_tensor_mul(NovaTensor *a, NovaTensor *b);
NovaTensor *nova_tensor_matmul(NovaTensor *a, NovaTensor *b);
NovaTensor *nova_tensor_relu(NovaTensor *a);
NovaTensor *nova_tensor_softmax(NovaTensor *a, int dim);
NovaTensor *nova_tensor_gelu(NovaTensor *a);
NovaTensor *nova_tensor_silu(NovaTensor *a);
NovaTensor *nova_tensor_tanh(NovaTensor *a);
NovaTensor *nova_tensor_sigmoid(NovaTensor *a);
NovaTensor *nova_tensor_clamp(NovaTensor *a, float min, float max);
NovaTensor *nova_tensor_pow(NovaTensor *a, float exponent);
NovaTensor *nova_tensor_transpose(NovaTensor *a, int dim0, int dim1);
NovaTensor *nova_tensor_unsqueeze(NovaTensor *a, int dim);

// Factories
NovaTensor *nova_tensor_randn(NovaContext *ctx, int64_t *shape, int ndim);
NovaTensor *nova_tensor_zeros(NovaContext *ctx, int64_t *shape, int ndim);
NovaTensor *nova_tensor_ones(NovaContext *ctx, int64_t *shape, int ndim);

// Debug
const char *nova_tensor_get_last_kernel(NovaTensor *t);

#endif // NOVA_ML_TENSOR_H

