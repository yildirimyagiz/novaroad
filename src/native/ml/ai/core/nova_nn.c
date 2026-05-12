#include "nova_nn.h"
#include "nova_tensor_ops.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LinearLayer *linear_create(int in_features, int out_features) {
  LinearLayer *layer = malloc(sizeof(LinearLayer));
  layer->in_features = in_features;
  layer->out_features = out_features;

  int64_t w_shape[] = {out_features, in_features};
  int64_t b_shape[] = {out_features};

  // We use NULL for context to use default context if applicable, or pass it if
  // required. NovaTensor create uses NULL context for global.
  layer->weight = nova_tensor_create(NULL, w_shape, 2, NOVA_DTYPE_FP32);
  layer->bias = nova_tensor_create(NULL, b_shape, 1, NOVA_DTYPE_FP32);

  // Initialize weights (Xavier initialization)
  float *w_data = (float *)layer->weight->data;
  float limit = sqrtf(6.0f / (in_features + out_features));
  for (size_t i = 0; i < layer->weight->total_elements; i++) {
    w_data[i] = ((float)rand() / (float)RAND_MAX) * 2.0f * limit - limit;
  }

  // Initialize bias to zero
  memset(layer->bias->data, 0, layer->bias->total_elements * sizeof(float));

  return layer;
}

void linear_free(LinearLayer *layer) {
  if (!layer)
    return;
  nova_tensor_destroy(layer->weight);
  nova_tensor_destroy(layer->bias);
  free(layer);
}

NovaTensor *linear_forward(LinearLayer *layer, NovaTensor *input) {
  // Y = XW^T + b
  // input shape: [batch, in_features]
  // weight shape: [out_features, in_features]
  // output shape: [batch, out_features]

  NovaTensor *w_t = nova_op_transpose(layer->weight, 0, 1);
  NovaTensor *out = nova_op_matmul(input, w_t);

  // Broadly add bias. out is [batch, out_features], bias is [out_features]
  // nova_op_add should handle broadcasting if implemented.
  NovaTensor *final_out = nova_op_add(out, layer->bias);

  // Cleanup intermediates
  nova_tensor_destroy(w_t);
  nova_tensor_destroy(out);

  return final_out;
}
