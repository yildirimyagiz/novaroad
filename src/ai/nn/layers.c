/**
 * @file layers.c
 * @brief Neural network layer implementations
 */

#include "ai/nn.h"
#include "ai/tensor.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Linear/Dense Layer */
typedef struct {
    nova_tensor_t *weights;
    nova_tensor_t *bias;
    size_t in_features;
    size_t out_features;
} linear_layer_t;

nova_layer_t *nova_layer_linear(size_t in_features, size_t out_features, bool use_bias)
{
    nova_layer_t *layer = nova_alloc(sizeof(nova_layer_t));
    if (!layer) return NULL;
    
    linear_layer_t *params = nova_alloc(sizeof(linear_layer_t));
    if (!params) {
        nova_free(layer);
        return NULL;
    }
    
    /* Initialize weights with Xavier initialization */
    size_t w_shape[2] = {out_features, in_features};
    params->weights = nova_tensor_randn(w_shape, 2, NOVA_DTYPE_FLOAT32);
    if (use_bias) {
        size_t b_shape[1] = {out_features};
        params->bias = nova_tensor_zeros(b_shape, 1, NOVA_DTYPE_FLOAT32);
    } else {
        params->bias = NULL;
    }
    params->in_features = in_features;
    params->out_features = out_features;
    
    layer->type = NOVA_LAYER_LINEAR;
    layer->params = params;
    layer->param_count = out_features * in_features + (use_bias ? out_features : 0);
    
    return layer;
}

/* Conv2D Layer */
typedef struct {
    nova_tensor_t *kernel;
    nova_tensor_t *bias;
    size_t in_channels;
    size_t out_channels;
    size_t kernel_size;
    size_t stride;
    size_t padding;
} conv2d_layer_t;

nova_layer_t *nova_layer_conv2d(size_t in_channels, size_t out_channels,
                                size_t kernel_size, size_t stride, size_t padding)
{
    nova_layer_t *layer = nova_alloc(sizeof(nova_layer_t));
    if (!layer) return NULL;
    
    conv2d_layer_t *params = nova_alloc(sizeof(conv2d_layer_t));
    if (!params) {
        nova_free(layer);
        return NULL;
    }
    
    /* He initialization for conv */
    size_t k_shape[4] = {out_channels, in_channels, kernel_size, kernel_size};
    params->kernel = nova_tensor_randn(k_shape, 4, NOVA_DTYPE_FLOAT32);
    size_t b_shape[1] = {out_channels};
    params->bias = nova_tensor_zeros(b_shape, 1, NOVA_DTYPE_FLOAT32);
    params->in_channels = in_channels;
    params->out_channels = out_channels;
    params->kernel_size = kernel_size;
    params->stride = stride;
    params->padding = padding;
    
    layer->type = NOVA_LAYER_CONV2D;
    layer->params = params;
    
    return layer;
}

/* Multi-Head Attention */
typedef struct {
    nova_tensor_t *Wq, *Wk, *Wv, *Wo;
    nova_tensor_t *bq, *bk, *bv, *bo;
    size_t embed_dim;
    size_t num_heads;
    size_t head_dim;
} attention_layer_t;

nova_layer_t *nova_layer_attention(size_t embed_dim, size_t num_heads, float dropout)
{
    (void)dropout;
    nova_layer_t *layer = nova_alloc(sizeof(nova_layer_t));
    if (!layer) return NULL;
    
    attention_layer_t *params = nova_alloc(sizeof(attention_layer_t));
    if (!params) {
        nova_free(layer);
        return NULL;
    }
    
    size_t head_dim = embed_dim / num_heads;
    size_t proj_shape[2] = {embed_dim, embed_dim};
    size_t bias_shape[1] = {embed_dim};
    
    /* Q, K, V projection matrices */
    params->Wq = nova_tensor_randn(proj_shape, 2, NOVA_DTYPE_FLOAT32);
    params->Wk = nova_tensor_randn(proj_shape, 2, NOVA_DTYPE_FLOAT32);
    params->Wv = nova_tensor_randn(proj_shape, 2, NOVA_DTYPE_FLOAT32);
    params->Wo = nova_tensor_randn(proj_shape, 2, NOVA_DTYPE_FLOAT32);
    
    params->bq = nova_tensor_zeros(bias_shape, 1, NOVA_DTYPE_FLOAT32);
    params->bk = nova_tensor_zeros(bias_shape, 1, NOVA_DTYPE_FLOAT32);
    params->bv = nova_tensor_zeros(bias_shape, 1, NOVA_DTYPE_FLOAT32);
    params->bo = nova_tensor_zeros(bias_shape, 1, NOVA_DTYPE_FLOAT32);
    
    params->embed_dim = embed_dim;
    params->num_heads = num_heads;
    params->head_dim = head_dim;
    
    layer->type = NOVA_LAYER_ATTENTION;
    layer->params = params;
    
    return layer;
}

/* Forward Pass */
nova_tensor_t *nova_layer_forward(nova_layer_t *layer, nova_tensor_t *input)
{
    if (!layer || !input) return NULL;
    
    switch (layer->type) {
        case NOVA_LAYER_LINEAR: {
            linear_layer_t *params = (linear_layer_t *)layer->params;
            nova_tensor_t *output = nova_tensor_matmul(input, params->weights);
            return nova_tensor_add(output, params->bias);
        }
        
        case NOVA_LAYER_CONV2D: {
            conv2d_layer_t *params = (conv2d_layer_t *)layer->params;
            // Perform convolution
            nova_tensor_t *conv_output = nova_tensor_conv2d(
                input, params->kernel, params->stride, params->padding);
            if (!conv_output) return NULL;
            
            // Add bias (broadcast across spatial dimensions)
            // For now, simple element-wise addition (assumes bias shape matches)
            return nova_tensor_add(conv_output, params->bias);
        }
        
        case NOVA_LAYER_ATTENTION: {
            attention_layer_t *params = (attention_layer_t *)layer->params;
            /* TODO: Implement multi-head attention */
            return input;
        }
        
        default:
            return NULL;
    }
}
