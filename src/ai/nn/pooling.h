/**
 * @file pooling.h
 * @brief Pooling layer operations
 */

#ifndef NOVA_AI_NN_POOLING_H
#define NOVA_AI_NN_POOLING_H

#include "ai/tensor.h"

typedef enum {
    NOVA_POOL_MAX,
    NOVA_POOL_AVG,
    NOVA_POOL_ADAPTIVE_MAX,
    NOVA_POOL_ADAPTIVE_AVG
} nova_pool_type_t;

/**
 * MaxPool2D operation
 * @param input Input tensor (batch, channels, height, width)
 * @param kernel_size Pooling window size
 * @param stride Stride for pooling
 * @param padding Padding size
 * @return Pooled tensor
 */
nova_tensor_t* nova_pool2d_max(nova_tensor_t* input, size_t kernel_size, size_t stride, size_t padding);

/**
 * AvgPool2D operation
 * @param input Input tensor (batch, channels, height, width)
 * @param kernel_size Pooling window size
 * @param stride Stride for pooling
 * @param padding Padding size
 * @return Pooled tensor
 */
nova_tensor_t* nova_pool2d_avg(nova_tensor_t* input, size_t kernel_size, size_t stride, size_t padding);

/**
 * AdaptiveMaxPool2D operation
 * @param input Input tensor (batch, channels, height, width)
 * @param output_height Target output height
 * @param output_width Target output width
 * @return Pooled tensor
 */
nova_tensor_t* nova_pool2d_adaptive_max(nova_tensor_t* input, size_t output_height, size_t output_width);

/**
 * AdaptiveAvgPool2D operation
 * @param input Input tensor (batch, channels, height, width)
 * @param output_height Target output height
 * @param output_width Target output width
 * @return Pooled tensor
 */
nova_tensor_t* nova_pool2d_adaptive_avg(nova_tensor_t* input, size_t output_height, size_t output_width);

/**
 * Global average pooling (reduce spatial dimensions)
 * @param input Input tensor (batch, channels, height, width)
 * @return Pooled tensor (batch, channels, 1, 1)
 */
nova_tensor_t* nova_pool2d_global_avg(nova_tensor_t* input);

#endif /* NOVA_AI_NN_POOLING_H */
