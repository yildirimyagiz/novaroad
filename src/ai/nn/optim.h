/**
 * @file optim.h
 * @brief Optimizer implementations
 */

#ifndef NOVA_AI_NN_OPTIM_H
#define NOVA_AI_NN_OPTIM_H

#include "ai/tensor.h"
#include <stdbool.h>

typedef enum {
    NOVA_OPTIM_SGD,
    NOVA_OPTIM_MOMENTUM,
    NOVA_OPTIM_ADAM,
    NOVA_OPTIM_ADAMW,
    NOVA_OPTIM_RMSPROP,
    NOVA_OPTIM_ADAGRAD
} nova_optim_type_t;

/* Optimizer state */
typedef struct nova_optimizer nova_optimizer_t;

/**
 * Create SGD optimizer
 * @param learning_rate Learning rate
 * @param momentum Momentum factor (0 for vanilla SGD)
 * @param weight_decay L2 regularization weight
 * @param nesterov Whether to use Nesterov momentum
 * @return Optimizer instance
 */
nova_optimizer_t* nova_optim_sgd(float learning_rate, float momentum, float weight_decay, bool nesterov);

/**
 * Create Adam optimizer
 * @param learning_rate Learning rate
 * @param beta1 Exponential decay rate for first moment
 * @param beta2 Exponential decay rate for second moment
 * @param eps Small constant for numerical stability
 * @param weight_decay L2 regularization weight
 * @return Optimizer instance
 */
nova_optimizer_t* nova_optim_adam(float learning_rate, float beta1, float beta2, float eps, float weight_decay);

/**
 * Create AdamW optimizer (Adam with decoupled weight decay)
 * @param learning_rate Learning rate
 * @param beta1 Exponential decay rate for first moment
 * @param beta2 Exponential decay rate for second moment
 * @param eps Small constant for numerical stability
 * @param weight_decay Weight decay factor
 * @return Optimizer instance
 */
nova_optimizer_t* nova_optim_adamw(float learning_rate, float beta1, float beta2, float eps, float weight_decay);

/**
 * Create RMSprop optimizer
 * @param learning_rate Learning rate
 * @param alpha Smoothing constant
 * @param eps Small constant for numerical stability
 * @param weight_decay L2 regularization weight
 * @param momentum Momentum factor
 * @return Optimizer instance
 */
nova_optimizer_t* nova_optim_rmsprop(float learning_rate, float alpha, float eps, float weight_decay, float momentum);

/**
 * Create Adagrad optimizer
 * @param learning_rate Learning rate
 * @param eps Small constant for numerical stability
 * @param weight_decay L2 regularization weight
 * @return Optimizer instance
 */
nova_optimizer_t* nova_optim_adagrad(float learning_rate, float eps, float weight_decay);

/**
 * Perform optimization step
 * @param optimizer Optimizer instance
 * @param params Parameters to update
 * @param grads Gradients
 * @param num_params Number of parameter tensors
 */
void nova_optim_step(nova_optimizer_t* optimizer, nova_tensor_t** params, nova_tensor_t** grads, size_t num_params);

/**
 * Zero gradients
 * @param optimizer Optimizer instance
 */
void nova_optim_zero_grad(nova_optimizer_t* optimizer);

/**
 * Get current learning rate
 * @param optimizer Optimizer instance
 * @return Current learning rate
 */
float nova_optim_get_lr(nova_optimizer_t* optimizer);

/**
 * Set learning rate
 * @param optimizer Optimizer instance
 * @param lr New learning rate
 */
void nova_optim_set_lr(nova_optimizer_t* optimizer, float lr);

/**
 * Free optimizer
 * @param optimizer Optimizer instance
 */
void nova_optim_free(nova_optimizer_t* optimizer);

#endif /* NOVA_AI_NN_OPTIM_H */
