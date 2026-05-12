#include "nova_tensor.h"
#include "formal/nova_proof_cache.h"
#include "nova_allocator.h"
#include "nova_context.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR IMPLEMENTATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Initialize random seed
static bool random_initialized = false;
static void nova_init_random(void) {
  if (!random_initialized) {
    srand(time(NULL));
    random_initialized = true;
  }
}

NovaTensor *nova_tensor_create(NovaContext *ctx, int64_t *shape, int ndim,
                                   NovaDType dtype) {
  return nova_tensor_create_on(ctx, shape, ndim, dtype, NOVA_DEVICE_CPU);
}

NovaTensor *nova_tensor_create_on(NovaContext *ctx, int64_t *shape,
                                      int ndim, NovaDType dtype,
                                      NovaDevice device) {

  NOVA_ASSERT_PROVEN(ndim > 0 && ndim <= 8, "tensor_valid_dimensions");

  NovaTensor *t =
      (NovaTensor *)nova_malloc(sizeof(NovaTensor), MEM_TAG_GENERAL);
  if (!t)
    return NULL;

  t->ndim = ndim;
  t->shape = (int64_t *)nova_malloc(ndim * sizeof(int64_t), MEM_TAG_GENERAL);
  t->strides =
      (int64_t *)nova_malloc(ndim * sizeof(int64_t), MEM_TAG_GENERAL);

  t->dtype = dtype;
  t->device = device;
  t->context = ctx;
  t->is_view = false;
  t->is_deterministic = ctx ? ctx->config.strict_determinism : false;
  t->grad = NULL;
  t->requires_grad = false;
  t->last_op_latency = 0.0;
  t->layout = NOVA_LAYOUT_CONTIGUOUS;

  // Debug Defaults
  t->creation_site = "nova_tensor_create";
  t->last_kernel_name = "none";
  t->last_backend_id = -1;
  t->last_invariant_check = true;

  t->total_elements = 1;
  for (int i = ndim - 1; i >= 0; i--) {
    t->shape[i] = shape[i];
    t->strides[i] = t->total_elements;
    t->total_elements *= shape[i];
  }

  size_t element_size = (dtype == NOVA_DTYPE_FP32) ? 4 : 2;
  if (dtype == NOVA_DTYPE_INT32)
    element_size = 4;

  // Alignment aware allocation
  t->data = nova_malloc_aligned(t->total_elements * element_size, 64,
                                  MEM_TAG_TENSOR);
  memset(t->data, 0, t->total_elements * element_size);

  return t;
}

NovaTensor *nova_tensor_from_data(void *data, int64_t *shape, int ndim,
                                      int dtype, bool copy) {

  if (copy) {
    NovaTensor *t =
        nova_tensor_create(NULL, shape, ndim, (NovaDType)dtype);
    size_t element_size =
        (t->dtype == NOVA_DTYPE_FP32 || t->dtype == NOVA_DTYPE_INT32) ? 4
                                                                          : 2;
    memcpy(t->data, data, t->total_elements * element_size);
    return t;
  } else {
    // Zero-copy wrap
    NovaTensor *t =
        (NovaTensor *)nova_malloc(sizeof(NovaTensor), MEM_TAG_GENERAL);
    t->ndim = ndim;
    t->shape =
        (int64_t *)nova_malloc(ndim * sizeof(int64_t), MEM_TAG_GENERAL);
    t->strides =
        (int64_t *)nova_malloc(ndim * sizeof(int64_t), MEM_TAG_GENERAL);
    memcpy(t->shape, shape, ndim * sizeof(int64_t));

    t->total_elements = 1;
    for (int i = ndim - 1; i >= 0; i--) {
      t->strides[i] = t->total_elements;
      t->total_elements *= shape[i];
    }

    t->dtype = (NovaDType)dtype;
    t->data = data;
    t->is_view = true;
    t->context = NULL;
    t->device = NOVA_DEVICE_CPU;
    t->requires_grad = false;
    t->grad = NULL;
    return t;
  }
}

void nova_tensor_destroy(NovaTensor *t) {
  if (!t)
    return;
  if (!t->is_view && t->data) {
    nova_free(t->data);
  }
  nova_free(t->shape);
  nova_free(t->strides);
  nova_free(t);
}

// Property accessors
void *nova_tensor_data(NovaTensor *t) { return t ? t->data : NULL; }
int64_t *nova_tensor_shape(NovaTensor *t) { return t ? t->shape : NULL; }
int nova_tensor_ndim(NovaTensor *t) { return t ? t->ndim : 0; }
size_t nova_tensor_size(NovaTensor *t) { return t ? t->total_elements : 0; }

// External declarations for ops
extern NovaTensor *nova_op_matmul(NovaTensor *a, NovaTensor *b);
extern NovaTensor *nova_op_add(NovaTensor *a, NovaTensor *b);

const char *nova_tensor_get_last_kernel(NovaTensor *t) {
  return t ? t->last_kernel_name : "null";
}

NovaTensor *nova_tensor_add(NovaTensor *a, NovaTensor *b) {
  NovaTensor *res = nova_op_add(a, b);
  res->last_kernel_name = "add";
  return res;
}

NovaTensor *nova_tensor_mul(NovaTensor *a, NovaTensor *b) {
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  float *ad = (float *)a->data;
  float *bd = (float *)b->data;
  float *rd = (float *)res->data;
  for (size_t i = 0; i < a->total_elements; i++)
    rd[i] = ad[i] * bd[i];
  return res;
}

NovaTensor *nova_tensor_matmul(NovaTensor *a, NovaTensor *b) {
  return nova_op_matmul(a, b);
}

NovaTensor *nova_tensor_relu(NovaTensor *a) {
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  float *ad = (float *)a->data;
  float *rd = (float *)res->data;
  for (size_t i = 0; i < a->total_elements; i++)
    rd[i] = ad[i] > 0 ? ad[i] : 0;
  return res;
}

NovaTensor *nova_tensor_softmax(NovaTensor *a, int dim) {
  (void)dim; // TODO: Implement dimension-specific softmax
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  float *ad = (float *)a->data;
  float *rd = (float *)res->data;

  // Numerical stable softmax implementation from todo
  float max_val = ad[0];
  for (size_t i = 1; i < a->total_elements; i++)
    if (ad[i] > max_val)
      max_val = ad[i];

  float sum = 0.0f;
  for (size_t i = 0; i < a->total_elements; i++) {
    rd[i] = expf(ad[i] - max_val);
    sum += rd[i];
  }
  for (size_t i = 0; i < a->total_elements; i++)
    rd[i] /= sum;

  return res;
}

// Factories
NovaTensor *nova_tensor_randn(NovaContext *ctx, int64_t *shape,
                                  int ndim) {
  nova_init_random();
  NovaTensor *t = nova_tensor_create(ctx, shape, ndim, NOVA_DTYPE_FP32);
  float *ptr = (float *)t->data;
  for (size_t i = 0; i < t->total_elements; i++) {
    // Basic Box-Muller transform
    float u1 = (float)rand() / RAND_MAX;
    float u2 = (float)rand() / RAND_MAX;
    ptr[i] = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
  }
  return t;
}

NovaTensor *nova_tensor_zeros(NovaContext *ctx, int64_t *shape,
                                  int ndim) {
  return nova_tensor_create(ctx, shape, ndim, NOVA_DTYPE_FP32);
}

NovaTensor *nova_tensor_ones(NovaContext *ctx, int64_t *shape, int ndim) {
  NovaTensor *t = nova_tensor_create(ctx, shape, ndim, NOVA_DTYPE_FP32);
  float *ptr = (float *)t->data;
  for (size_t i = 0; i < t->total_elements; i++)
    ptr[i] = 1.0f;
  return t;
}
