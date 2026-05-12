/**
 * @file nn.h
 * @brief Neural network layers and operations
 */

#ifndef NOVA_NN_H
#define NOVA_NN_H

#include "tensor.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Layer Types
 * ========================================================================== */

typedef enum {
    NOVA_LAYER_LINEAR,
    NOVA_LAYER_CONV2D,
    NOVA_LAYER_MAXPOOL2D,
    NOVA_LAYER_DROPOUT,
    NOVA_LAYER_BATCHNORM,
    NOVA_LAYER_ATTENTION,
    NOVA_LAYER_EMBEDDING,
    NOVA_LAYER_LSTM,
    NOVA_LAYER_GRU,
} nova_layer_type_t;

typedef struct nova_layer nova_layer_t;

struct nova_layer {
    nova_layer_type_t type;
    void *params;
    size_t param_count;
};

/* ============================================================================
 * Layer Creation
 * ========================================================================== */

/**
 * Create linear (fully connected) layer
 * @param in_features Input feature dimension
 * @param out_features Output feature dimension
 * @param use_bias Whether to use bias term
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_linear(size_t in_features, size_t out_features, bool use_bias);

/**
 * Create 2D convolutional layer
 * @param in_channels Number of input channels
 * @param out_channels Number of output channels
 * @param kernel_size Kernel size (square)
 * @param stride Stride
 * @param padding Padding
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_conv2d(size_t in_channels, size_t out_channels,
                                size_t kernel_size, size_t stride, size_t padding);

/**
 * Create max pooling layer
 * @param kernel_size Pooling kernel size
 * @param stride Stride
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_maxpool2d(size_t kernel_size, size_t stride);

/**
 * Create dropout layer
 * @param dropout_rate Probability of dropping (0.0 to 1.0)
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_dropout(float dropout_rate);

/**
 * Create batch normalization layer
 * @param num_features Number of features to normalize
 * @param eps Small constant for numerical stability
 * @param momentum Momentum for running statistics
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_batchnorm(size_t num_features, float eps, float momentum);

/**
 * Create multi-head self-attention layer
 * @param embed_dim Embedding dimension
 * @param num_heads Number of attention heads
 * @param dropout Dropout probability
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_attention(size_t embed_dim, size_t num_heads, float dropout);

/**
 * Create embedding layer
 * @param num_embeddings Vocabulary size
 * @param embedding_dim Embedding dimension
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_embedding(size_t num_embeddings, size_t embedding_dim);

/**
 * Create LSTM layer
 * @param input_size Input feature size
 * @param hidden_size Hidden state size
 * @param num_layers Number of stacked LSTM layers
 * @param bidirectional Whether to use bidirectional LSTM
 * @return Created layer or NULL on error
 */
nova_layer_t *nova_layer_lstm(size_t input_size, size_t hidden_size,
                              size_t num_layers, bool bidirectional);

/* ============================================================================
 * Layer Operations
 * ========================================================================== */

/**
 * Forward pass through layer
 * @param layer The layer
 * @param input Input tensor
 * @return Output tensor or NULL on error
 */
nova_tensor_t *nova_layer_forward(nova_layer_t *layer, nova_tensor_t *input);

/**
 * Set layer to training mode
 * @param layer The layer
 */
void nova_layer_train(nova_layer_t *layer);

/**
 * Set layer to evaluation mode
 * @param layer The layer
 */
void nova_layer_eval(nova_layer_t *layer);

/**
 * Get layer parameters
 * @param layer The layer
 * @param num_params Output: number of parameters
 * @return Array of parameter tensors
 */
nova_tensor_t **nova_layer_parameters(nova_layer_t *layer, size_t *num_params);

/**
 * Destroy layer and free resources
 * @param layer The layer to destroy
 */
void nova_layer_destroy(nova_layer_t *layer);

/* ============================================================================
 * Activation Functions
 * ========================================================================== */

/**
 * ReLU activation: max(0, x)
 * @param x Input tensor
 * @return Output tensor
 */
nova_tensor_t *nova_relu(nova_tensor_t *x);

/**
 * Leaky ReLU activation: max(alpha * x, x)
 * @param x Input tensor
 * @param alpha Negative slope
 * @return Output tensor
 */
nova_tensor_t *nova_leaky_relu(nova_tensor_t *x, float alpha);

/**
 * GELU activation (Gaussian Error Linear Unit)
 * @param x Input tensor
 * @return Output tensor
 */
nova_tensor_t *nova_gelu(nova_tensor_t *x);

/**
 * Sigmoid activation: 1 / (1 + exp(-x))
 * @param x Input tensor
 * @return Output tensor
 */
nova_tensor_t *nova_sigmoid(nova_tensor_t *x);

/**
 * Tanh activation: (exp(x) - exp(-x)) / (exp(x) + exp(-x))
 * @param x Input tensor
 * @return Output tensor
 */
nova_tensor_t *nova_tanh(nova_tensor_t *x);

/**
 * Softmax activation along specified dimension
 * @param x Input tensor
 * @param dim Dimension to apply softmax
 * @return Output tensor
 */
nova_tensor_t *nova_softmax(nova_tensor_t *x, int dim);

/**
 * Log-softmax activation (numerically stable)
 * @param x Input tensor
 * @param dim Dimension to apply log-softmax
 * @return Output tensor
 */
nova_tensor_t *nova_log_softmax(nova_tensor_t *x, int dim);

/* ============================================================================
 * Loss Functions
 * ========================================================================== */

/**
 * Mean squared error loss
 * @param predicted Predicted values
 * @param target Target values
 * @return Scalar loss tensor
 */
nova_tensor_t *nova_mse_loss(nova_tensor_t *predicted, nova_tensor_t *target);

/**
 * Cross-entropy loss
 * @param predicted Predicted logits
 * @param target Target class indices
 * @return Scalar loss tensor
 */
nova_tensor_t *nova_cross_entropy_loss(nova_tensor_t *predicted, nova_tensor_t *target);

/**
 * Binary cross-entropy loss
 * @param predicted Predicted probabilities
 * @param target Target binary labels
 * @return Scalar loss tensor
 */
nova_tensor_t *nova_bce_loss(nova_tensor_t *predicted, nova_tensor_t *target);

/**
 * Negative log-likelihood loss
 * @param predicted Log probabilities
 * @param target Target class indices
 * @return Scalar loss tensor
 */
nova_tensor_t *nova_nll_loss(nova_tensor_t *predicted, nova_tensor_t *target);

/* ============================================================================
 * Normalization
 * ========================================================================== */

/**
 * Layer normalization
 * @param x Input tensor
 * @param normalized_shape Shape to normalize over
 * @param eps Epsilon for numerical stability
 * @return Normalized tensor
 */
nova_tensor_t *nova_layer_norm(nova_tensor_t *x, size_t *normalized_shape,
                               size_t ndim, float eps);

/**
 * Group normalization
 * @param x Input tensor
 * @param num_groups Number of groups
 * @param eps Epsilon for numerical stability
 * @return Normalized tensor
 */
nova_tensor_t *nova_group_norm(nova_tensor_t *x, size_t num_groups, float eps);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_NN_H */
