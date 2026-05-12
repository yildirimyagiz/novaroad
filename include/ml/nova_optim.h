#ifndef NOVA_OPTIM_H
#define NOVA_OPTIM_H

#include "nova_tensor.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA OPTIM - Machine Learning Optimizers
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Full implementations of modern ML optimizers:
 * - AdamW (Adam with decoupled weight decay)
 * - SGD (Stochastic Gradient Descent with momentum)
 * - RMSprop (Root Mean Square Propagation)
 */

// ═══════════════════════════════════════════════════════════════════════════
// AdamW Optimizer (PyTorch-style with decoupled weight decay)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct NovaAdamWOptimizer {
    float lr;              // Learning rate
    float beta1;           // Exponential decay rate for 1st moment (default: 0.9)
    float beta2;           // Exponential decay rate for 2nd moment (default: 0.999)
    float eps;             // Small constant for numerical stability (default: 1e-8)
    float weight_decay;    // Weight decay coefficient (default: 0.01)
    int step;              // Current step number
    
    // Optimizer state
    int num_params;        // Number of parameters
    NovaTensor **m_states; // First moment estimates (momentum)
    NovaTensor **v_states; // Second moment estimates (variance)
} NovaAdamWOptimizer;

/**
 * Create AdamW optimizer
 * 
 * @param lr Learning rate (typical: 1e-3 to 3e-4)
 * @param beta1 First moment decay (default: 0.9)
 * @param beta2 Second moment decay (default: 0.999)
 * @param eps Epsilon for numerical stability (default: 1e-8)
 * @param weight_decay Weight decay coefficient (default: 0.01)
 * @return Initialized AdamW optimizer
 */
NovaAdamWOptimizer *nova_adamw_create(float lr, float beta1, float beta2, float eps, 
                                      float weight_decay);

/**
 * Initialize optimizer state for a set of parameters
 * 
 * @param opt AdamW optimizer
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 */
void nova_adamw_init_state(NovaAdamWOptimizer *opt, NovaTensor **params, int num_params);

/**
 * Perform single optimization step
 * 
 * Algorithm (from PyTorch AdamW):
 *   for each parameter p with gradient g:
 *     // Decoupled weight decay (key difference from Adam)
 *     p = p * (1 - lr * weight_decay)
 *     
 *     // Momentum update
 *     m = beta1 * m + (1 - beta1) * g
 *     v = beta2 * v + (1 - beta2) * g^2
 *     
 *     // Bias correction
 *     m_hat = m / (1 - beta1^step)
 *     v_hat = v / (1 - beta2^step)
 *     
 *     // Parameter update
 *     p = p - lr * m_hat / (sqrt(v_hat) + eps)
 * 
 * @param opt AdamW optimizer
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 */
void nova_adamw_step(NovaAdamWOptimizer *opt, NovaTensor **params, int num_params);

/**
 * Zero all parameter gradients
 * 
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 */
void nova_adamw_zero_grad(NovaTensor **params, int num_params);

/**
 * Free optimizer and its state
 * 
 * @param opt AdamW optimizer to free
 */
void nova_adamw_free(NovaAdamWOptimizer *opt);

// ═══════════════════════════════════════════════════════════════════════════
// SGD Optimizer (with Nesterov momentum)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float lr;              // Learning rate
    float momentum;        // Momentum factor (default: 0.9)
    float dampening;       // Dampening for momentum (default: 0.0)
    float weight_decay;    // L2 penalty (default: 0.0)
    bool nesterov;         // Use Nesterov momentum (default: false)
    
    // Optimizer state
    int num_params;
    NovaTensor **momentum_buffer; // Momentum buffers for each parameter
} NovaSGD;

/**
 * Create SGD optimizer
 * 
 * @param lr Learning rate
 * @param momentum Momentum factor (0.0 for vanilla SGD)
 * @param weight_decay L2 regularization coefficient
 * @param nesterov Use Nesterov accelerated gradient
 * @return Initialized SGD optimizer
 */
NovaSGD *nova_sgd_create(float lr, float momentum, float weight_decay, bool nesterov);

/**
 * Initialize SGD state for parameters
 * 
 * @param opt SGD optimizer
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 */
void nova_sgd_init_state(NovaSGD *opt, NovaTensor **params, int num_params);

/**
 * Perform SGD optimization step
 * 
 * Algorithm:
 *   for each parameter p with gradient g:
 *     // L2 weight decay
 *     if weight_decay != 0:
 *       g = g + weight_decay * p
 *     
 *     // Momentum update
 *     if momentum != 0:
 *       buf = momentum * buf + (1 - dampening) * g
 *       if nesterov:
 *         g = g + momentum * buf  // Nesterov acceleration
 *       else:
 *         g = buf
 *     
 *     // Parameter update
 *     p = p - lr * g
 * 
 * @param opt SGD optimizer
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 */
void nova_sgd_step(NovaSGD *opt, NovaTensor **params, int num_params);

/**
 * Zero all parameter gradients
 * 
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 */
void nova_sgd_zero_grad(NovaTensor **params, int num_params);

/**
 * Free SGD optimizer
 * 
 * @param opt SGD optimizer to free
 */
void nova_sgd_free(NovaSGD *opt);

// ═══════════════════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Clip gradient norms (for training stability)
 * 
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 * @param max_norm Maximum norm value
 */
void nova_clip_grad_norm(NovaTensor **params, int num_params, float max_norm);

/**
 * Get total gradient norm across all parameters
 * 
 * @param params Array of parameter tensors
 * @param num_params Number of parameters
 * @return L2 norm of all gradients
 */
float nova_get_grad_norm(NovaTensor **params, int num_params);

#endif // NOVA_OPTIM_H
