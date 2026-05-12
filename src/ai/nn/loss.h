/**
 * @file loss.h
 * @brief Loss function implementations
 */

#ifndef NOVA_AI_NN_LOSS_H
#define NOVA_AI_NN_LOSS_H

#include "ai/tensor.h"

typedef enum {
    NOVA_LOSS_MSE,              // Mean Squared Error
    NOVA_LOSS_MAE,              // Mean Absolute Error
    NOVA_LOSS_CROSS_ENTROPY,    // Cross Entropy
    NOVA_LOSS_BCE,              // Binary Cross Entropy
    NOVA_LOSS_BCE_LOGITS,       // BCE with Logits
    NOVA_LOSS_NLL,              // Negative Log Likelihood
    NOVA_LOSS_HUBER,            // Huber Loss
    NOVA_LOSS_SMOOTH_L1,        // Smooth L1 Loss
    NOVA_LOSS_KL_DIV            // KL Divergence
} nova_loss_type_t;

/**
 * Mean Squared Error Loss
 * @param predictions Predicted values
 * @param targets Target values
 * @return Scalar loss value
 */
float nova_loss_mse(nova_tensor_t* predictions, nova_tensor_t* targets);

/**
 * Mean Absolute Error Loss
 * @param predictions Predicted values
 * @param targets Target values
 * @return Scalar loss value
 */
float nova_loss_mae(nova_tensor_t* predictions, nova_tensor_t* targets);

/**
 * Cross Entropy Loss (with softmax)
 * @param logits Raw predictions (before softmax)
 * @param targets Target class indices or one-hot vectors
 * @return Scalar loss value
 */
float nova_loss_cross_entropy(nova_tensor_t* logits, nova_tensor_t* targets);

/**
 * Binary Cross Entropy Loss
 * @param predictions Predicted probabilities (0-1)
 * @param targets Target binary values (0 or 1)
 * @return Scalar loss value
 */
float nova_loss_bce(nova_tensor_t* predictions, nova_tensor_t* targets);

/**
 * Binary Cross Entropy with Logits
 * @param logits Raw predictions (before sigmoid)
 * @param targets Target binary values (0 or 1)
 * @return Scalar loss value
 */
float nova_loss_bce_with_logits(nova_tensor_t* logits, nova_tensor_t* targets);

/**
 * Negative Log Likelihood Loss
 * @param log_probs Log probabilities
 * @param targets Target class indices
 * @return Scalar loss value
 */
float nova_loss_nll(nova_tensor_t* log_probs, nova_tensor_t* targets);

/**
 * Huber Loss (smooth L1 alternative)
 * @param predictions Predicted values
 * @param targets Target values
 * @param delta Threshold parameter
 * @return Scalar loss value
 */
float nova_loss_huber(nova_tensor_t* predictions, nova_tensor_t* targets, float delta);

/**
 * Smooth L1 Loss
 * @param predictions Predicted values
 * @param targets Target values
 * @return Scalar loss value
 */
float nova_loss_smooth_l1(nova_tensor_t* predictions, nova_tensor_t* targets);

/**
 * KL Divergence Loss
 * @param predictions Predicted distribution
 * @param targets Target distribution
 * @return Scalar loss value
 */
float nova_loss_kl_div(nova_tensor_t* predictions, nova_tensor_t* targets);

#endif /* NOVA_AI_NN_LOSS_H */
