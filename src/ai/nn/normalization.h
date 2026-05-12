/**
 * @file normalization.h
 * @brief Normalization layer operations
 */

#ifndef NOVA_AI_NN_NORMALIZATION_H
#define NOVA_AI_NN_NORMALIZATION_H

#include "ai/tensor.h"

/**
 * Layer Normalization
 * Normalizes across the last dimension(s)
 * @param input Input tensor
 * @param normalized_shape Number of dimensions to normalize over
 * @param eps Small constant for numerical stability
 * @return Normalized tensor
 */
nova_tensor_t* nova_layer_norm(nova_tensor_t* input, size_t normalized_shape, float eps);

/**
 * Batch Normalization
 * Normalizes across batch dimension
 * @param input Input tensor (batch, channels, height, width) or (batch, features)
 * @param running_mean Running mean for inference (can be NULL for training)
 * @param running_var Running variance for inference (can be NULL for training)
 * @param gamma Scale parameter
 * @param beta Shift parameter
 * @param eps Small constant for numerical stability
 * @param momentum Momentum for running statistics update
 * @param training Whether in training mode
 * @return Normalized tensor
 */
nova_tensor_t* nova_batch_norm(nova_tensor_t* input, 
                               nova_tensor_t* running_mean,
                               nova_tensor_t* running_var,
                               nova_tensor_t* gamma,
                               nova_tensor_t* beta,
                               float eps,
                               float momentum,
                               bool training);

/**
 * Group Normalization
 * Divides channels into groups and normalizes within each group
 * @param input Input tensor (batch, channels, height, width)
 * @param num_groups Number of groups
 * @param eps Small constant for numerical stability
 * @return Normalized tensor
 */
nova_tensor_t* nova_group_norm(nova_tensor_t* input, size_t num_groups, float eps);

/**
 * Instance Normalization
 * Normalizes each channel independently
 * @param input Input tensor (batch, channels, height, width)
 * @param eps Small constant for numerical stability
 * @return Normalized tensor
 */
nova_tensor_t* nova_instance_norm(nova_tensor_t* input, float eps);

/**
 * RMS Normalization (Root Mean Square)
 * Used in modern transformers (e.g., LLaMA)
 * @param input Input tensor
 * @param eps Small constant for numerical stability
 * @return Normalized tensor
 */
nova_tensor_t* nova_rms_norm(nova_tensor_t* input, float eps);

#endif /* NOVA_AI_NN_NORMALIZATION_H */
