#ifndef NOVA_NN_H
#define NOVA_NN_H

#include "nova_tensor.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA NN - Common Neural Network Components & Bridge
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Core types alias
typedef NovaTensor Tensor;
typedef NovaDType DType;

// Descriptor for model metadata
typedef struct NovaTensorDescriptor {
  NovaTensor *tensor;
  char name[64];
} NovaTensorDescriptor;

// Layer Definitions
typedef struct {
  NovaTensor *weight;
  NovaTensor *bias;
  int in_features;
  int out_features;
} LinearLayer;

typedef struct {
  NovaTensor *weight;
  NovaTensor *bias;
  NovaConvMode conv_mode;
  int in_channels;
  int out_channels;
  int kernel_size;
  int stride;
  int padding;
  int dilation;
  NovaActivationType activation;
  NovaNormType normalization;
} Conv2D;

typedef Conv2D ConvLayer; // Alias for CNN models

// Optimizer bridge
typedef struct {
  float lr;
  float beta1;
  float beta2;
  float epsilon;
  float weight_decay;
  int step;
} AdamW;

// CLIP placeholder
typedef struct {
  int vocab_size;
  int max_length;
} CLIPTokenizer;

// Layer API
LinearLayer *linear_create(int in_features, int out_features);
void linear_free(LinearLayer *layer);
NovaTensor *linear_forward(LinearLayer *layer, NovaTensor *input);

// Tensor Operation Aliases (Experimental Bridge)
static inline NovaTensor *tensor_create(float *data, int *shape, int ndim,
                                          int grad) {
  int64_t shape64[8];
  for (int i = 0; i < ndim; i++)
    shape64[i] = (int64_t)shape[i];
  NovaTensor *t =
      nova_tensor_create(NULL, shape64, ndim, NOVA_DTYPE_FP32);
  if (data)
    memcpy(t->data, data, t->total_elements * sizeof(float));
  t->requires_grad = (bool)grad;
  return t;
}

static inline void tensor_free(NovaTensor *t) { nova_tensor_destroy(t); }
static inline NovaTensor *tensor_add(NovaTensor *a, NovaTensor *b) {
  return nova_tensor_add(a, b);
}
static inline NovaTensor *tensor_matmul(NovaTensor *a, NovaTensor *b) {
  return nova_tensor_matmul(a, b);
}
static inline NovaTensor *tensor_relu(NovaTensor *a) {
  return nova_tensor_relu(a);
}
static inline NovaTensor *tensor_transpose(NovaTensor *t, int d1, int d2) {
  (void)d1;
  (void)d2;
  // In actual implementation would return a view
  return t;
}
static inline NovaTensor *tensor_scalar_mul(NovaTensor *t, float s) {
  // Basic scalar mul implementation
  float *data = (float *)t->data;
  for (size_t i = 0; i < t->total_elements; i++)
    data[i] *= s;
  return t;
}

// Optimizer API
static inline AdamW *adamw_create(float lr) {
  AdamW *opt = malloc(sizeof(AdamW));
  opt->lr = lr;
  opt->beta1 = 0.9f;
  opt->beta2 = 0.999f;
  opt->epsilon = 1e-8f;
  opt->weight_decay = 0.01f;
  opt->step = 0;
  return opt;
}
static inline void adamw_free(AdamW *opt) { free(opt); }
static inline void adamw_step(AdamW *opt, NovaTensor *p) {
  opt->step++;
  if (p->grad) {
    float *data = (float *)p->data;
    float *grad = (float *)p->grad->data;
    for (size_t i = 0; i < p->total_elements; i++)
      data[i] -= opt->lr * grad[i];
  }
}

#endif // NOVA_NN_H
