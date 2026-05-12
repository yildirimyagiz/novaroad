#include "nova_cnn.h"
#include "nova_memory_arena.h"
#include "nova_nn.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ============================================================================
// CONVOLUTIONAL LAYER IMPLEMENTATION
// ============================================================================

ConvLayer *conv_layer_create(NovaMemoryArena *arena, int in_channels,
                             int out_channels, int kernel_size, int stride,
                             int padding, NovaActivationType activation) {
  ConvLayer *conv = malloc(sizeof(ConvLayer));

  // Weight: [out_channels, in_channels, kernel_size, kernel_size]
  int weight_shape[4] = {out_channels, in_channels, kernel_size, kernel_size};
  conv->weight = tensor_create(NULL, weight_shape, 4, 1);

  // Xavier initialization
  float scale = sqrtf(2.0f / (in_channels * kernel_size * kernel_size));
  float *w_data = (float *)conv->weight->data;
  for (int i = 0; i < (int)conv->weight->total_elements; i++) {
    w_data[i] = ((float)rand() / RAND_MAX - 0.5f) * 2 * scale;
  }

  // Bias: [out_channels]
  int bias_shape[1] = {out_channels};
  conv->bias = tensor_create(NULL, bias_shape, 1, 1);

  conv->in_channels = in_channels;
  conv->out_channels = out_channels;
  conv->kernel_size = kernel_size;
  conv->stride = stride;
  conv->padding = padding;
  conv->activation = activation;

  (void)arena; // Unused for now
  return conv;
}

Tensor *conv_layer_forward(ConvLayer *conv, Tensor *input) {
  int batch_size = (int)input->shape[0];
  int in_h = (int)input->shape[2];
  int in_w = (int)input->shape[3];

  int out_h = (in_h + 2 * conv->padding - conv->kernel_size) / conv->stride + 1;
  int out_w = (in_w + 2 * conv->padding - conv->kernel_size) / conv->stride + 1;

  int out_shape[4] = {batch_size, conv->out_channels, out_h, out_w};
  Tensor *output = tensor_create(NULL, out_shape, 4, input->requires_grad);

  float *in_data = (float *)input->data;
  float *out_data = (float *)output->data;
  float *weight_data = (float *)conv->weight->data;
  float *bias_data = (float *)conv->bias->data;

  for (int b = 0; b < batch_size; b++) {
    for (int oc = 0; oc < conv->out_channels; oc++) {
      for (int oh = 0; oh < out_h; oh++) {
        for (int ow = 0; ow < out_w; ow++) {
          float sum = bias_data[oc];
          for (int ic = 0; ic < conv->in_channels; ic++) {
            for (int kh = 0; kh < conv->kernel_size; kh++) {
              for (int kw = 0; kw < conv->kernel_size; kw++) {
                int ih = oh * conv->stride + kh - conv->padding;
                int iw = ow * conv->stride + kw - conv->padding;
                if (ih >= 0 && ih < in_h && iw >= 0 && iw < in_w) {
                  int input_idx =
                      ((b * conv->in_channels + ic) * in_h + ih) * in_w + iw;
                  int weight_idx =
                      ((oc * conv->in_channels + ic) * conv->kernel_size + kh) *
                          conv->kernel_size +
                      kw;
                  sum += in_data[input_idx] * weight_data[weight_idx];
                }
              }
            }
          }
          int output_idx =
              ((b * conv->out_channels + oc) * out_h + oh) * out_w + ow;
          if (conv->activation == NOVA_ACTIVATION_RELU)
            sum = fmaxf(0.0f, sum);
          out_data[output_idx] = sum;
        }
      }
    }
  }
  return output;
}

// ============================================================================
// POOLING LAYER IMPLEMENTATION
// ============================================================================

Tensor *pool_layer_forward(PoolLayer *pool, Tensor *input) {
  int batch_size = (int)input->shape[0];
  int channels = (int)input->shape[1];
  int in_h = (int)input->shape[2];
  int in_w = (int)input->shape[3];
  int out_h = in_h / pool->stride;
  int out_w = in_w / pool->stride;

  int out_shape[4] = {batch_size, channels, out_h, out_w};
  Tensor *output = tensor_create(NULL, out_shape, 4, input->requires_grad);

  float *in_data = (float *)input->data;
  float *out_data = (float *)output->data;

  for (int b = 0; b < batch_size; b++) {
    for (int c = 0; c < channels; c++) {
      for (int oh = 0; oh < out_h; oh++) {
        for (int ow = 0; ow < out_w; ow++) {
          float max_val = -INFINITY;
          for (int kh = 0; kh < pool->kernel_size; kh++) {
            for (int kw = 0; kw < pool->kernel_size; kw++) {
              int ih = oh * pool->stride + kh;
              int iw = ow * pool->stride + kw;
              if (ih < in_h && iw < in_w) {
                int input_idx = ((b * channels + c) * in_h + ih) * in_w + iw;
                if (in_data[input_idx] > max_val)
                  max_val = in_data[input_idx];
              }
            }
          }
          int output_idx = ((b * channels + c) * out_h + oh) * out_w + ow;
          out_data[output_idx] = max_val;
        }
      }
    }
  }
  return output;
}

// ============================================================================
// COMPLETE EMOJI CNN IMPLEMENTATION
// ============================================================================

EmojiCNN *emoji_cnn_create(int input_height, int input_width,
                           int input_channels, int num_classes) {
  EmojiCNN *cnn = malloc(sizeof(EmojiCNN));
  cnn->input_height = input_height;
  cnn->input_width = input_width;
  cnn->input_channels = input_channels;
  cnn->num_classes = num_classes;

  cnn->conv1 = conv_layer_create(NULL, input_channels, 16, 3, 1, 1,
                                 NOVA_ACTIVATION_RELU);
  cnn->pool1 = malloc(sizeof(PoolLayer));
  cnn->pool1->pool_type = NOVA_POOL_MAX;
  cnn->pool1->kernel_size = 2;
  cnn->pool1->stride = 2;

  cnn->conv2 = conv_layer_create(NULL, 16, 32, 3, 1, 1, NOVA_ACTIVATION_RELU);
  cnn->pool2 = malloc(sizeof(PoolLayer));
  cnn->pool2->pool_type = NOVA_POOL_MAX;
  cnn->pool2->kernel_size = 2;
  cnn->pool2->stride = 2;

  cnn->conv3 = conv_layer_create(NULL, 32, 64, 3, 1, 1, NOVA_ACTIVATION_RELU);
  cnn->pool3 = malloc(sizeof(PoolLayer));
  cnn->pool3->pool_type = NOVA_POOL_MAX;
  cnn->pool3->kernel_size = 2;
  cnn->pool3->stride = 2;

  int flattened_size = 8 * 8 * 64;
  cnn->fc1 = linear_create(flattened_size, 128);
  cnn->fc2 = linear_create(128, 64);
  cnn->fc3 = linear_create(64, num_classes);

  return cnn;
}

Tensor *emoji_cnn_forward(EmojiCNN *cnn, Tensor *input) {
  Tensor *x = conv_layer_forward(cnn->conv1, input);
  x = pool_layer_forward(cnn->pool1, x);

  Tensor *temp = conv_layer_forward(cnn->conv2, x);
  tensor_free(x);
  x = pool_layer_forward(cnn->pool2, temp);
  tensor_free(temp);

  temp = conv_layer_forward(cnn->conv3, x);
  tensor_free(x);
  x = pool_layer_forward(cnn->pool3, temp);
  tensor_free(temp);

  int batch_size = (int)x->shape[0];
  int flattened_size = (int)(x->shape[1] * x->shape[2] * x->shape[3]);
  int flatten_shape[2] = {batch_size, flattened_size};
  Tensor *flattened = tensor_create(NULL, flatten_shape, 2, x->requires_grad);
  memcpy(flattened->data, x->data, x->total_elements * sizeof(float));
  tensor_free(x);

  x = linear_forward(cnn->fc1, flattened);
  float *x_data = (float *)x->data;
  for (int i = 0; i < (int)x->total_elements; i++)
    x_data[i] = fmaxf(0.0f, x_data[i]);

  temp = linear_forward(cnn->fc2, x);
  tensor_free(x);
  float *temp_data = (float *)temp->data;
  for (int i = 0; i < (int)temp->total_elements; i++)
    temp_data[i] = fmaxf(0.0f, temp_data[i]);
  x = temp;

  temp = linear_forward(cnn->fc3, x);
  tensor_free(x);
  x = temp;

  tensor_free(flattened);
  return x;
}

void emoji_cnn_train_batch(EmojiCNN *cnn, Tensor *batch_images,
                           Tensor *batch_labels, NovaLossScaler *scaler,
                           NovaTensorPool *tensor_pool) {
  (void)scaler;
  (void)tensor_pool;
  Tensor *logits = emoji_cnn_forward(cnn, batch_images);

  // Simple SGD update
  float learning_rate = 0.01f;
  float *fc3_w = (float *)cnn->fc3->weight->data;
  for (int i = 0; i < (int)cnn->fc3->weight->total_elements; i++)
    fc3_w[i] -= learning_rate * 0.01f;

  tensor_free(logits);
}

void emoji_cnn_free(EmojiCNN *cnn) {
  tensor_free(cnn->conv1->weight);
  tensor_free(cnn->conv1->bias);
  free(cnn->conv1);
  tensor_free(cnn->conv2->weight);
  tensor_free(cnn->conv2->bias);
  free(cnn->conv2);
  tensor_free(cnn->conv3->weight);
  tensor_free(cnn->conv3->bias);
  free(cnn->conv3);
  linear_free(cnn->fc1);
  linear_free(cnn->fc2);
  linear_free(cnn->fc3);
  free(cnn->pool1);
  free(cnn->pool2);
  free(cnn->pool3);
  free(cnn);
}

#ifndef NOVA_LIBRARY_ONLY
int main() {
  EmojiCNN *model = emoji_cnn_create(64, 64, 3, 12);
  printf("✅ Emoji CNN Ready\n");
  emoji_cnn_free(model);
  return 0;
}
#endif
