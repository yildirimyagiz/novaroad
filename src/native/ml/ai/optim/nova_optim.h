/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA OPTIM - Optimizer Header
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Common types and interfaces for Nova optimizers:
 * - SGD (with momentum)
 * - AdamW (adaptive momentum with weight decay)
 */

#ifndef NOVA_OPTIM_H
#define NOVA_OPTIM_H

#include "ml/nova_tensor.h"
#include <stdbool.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * SGD Optimizer
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    float lr;                    /* learning rate */
    float momentum;              /* momentum factor */
    float dampening;             /* gradient dampening (default 0.0) */
    float weight_decay;          /* L2 regularization coefficient */
    bool nesterov;               /* enable Nesterov acceleration */
    int num_params;              /* number of parameters */
    NovaTensor **momentum_buffer; /* momentum buffers per parameter */
} NovaSGD;

/**
 * Create an SGD optimizer
 * @param lr: learning rate (e.g., 0.01)
 * @param momentum: momentum factor (e.g., 0.9, or 0 for vanilla SGD)
 * @param weight_decay: L2 regularization (e.g., 1e-4, or 0 for none)
 * @param nesterov: enable Nesterov acceleration
 * @return: allocated optimizer or NULL on failure
 */
NovaSGD *nova_sgd_create(float lr, float momentum, float weight_decay, bool nesterov);

/**
 * Initialize optimizer state (momentum buffers)
 * @param opt: optimizer
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 */
void nova_sgd_init_state(NovaSGD *opt, NovaTensor **params, int num_params);

/**
 * Apply one SGD step to all parameters
 * Assumes parameters have .grad set
 * @param opt: optimizer
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 */
void nova_sgd_step(NovaSGD *opt, NovaTensor **params, int num_params);

/**
 * Zero out all gradients
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 */
void nova_sgd_zero_grad(NovaTensor **params, int num_params);

/**
 * Free optimizer resources
 * @param opt: optimizer to free
 */
void nova_sgd_free(NovaSGD *opt);

/* ═══════════════════════════════════════════════════════════════════════════
 * AdamW Optimizer
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    float lr;                 /* learning rate (e.g., 1e-3) */
    float beta1;              /* first moment decay (e.g., 0.9) */
    float beta2;              /* second moment decay (e.g., 0.999) */
    float eps;                /* numerical stability (e.g., 1e-8) */
    float weight_decay;       /* decoupled weight decay (e.g., 1e-4) */
    int step;                 /* global step counter */
    int num_params;           /* number of parameters */
    NovaTensor **m_states;    /* first moment estimates per param */
    NovaTensor **v_states;    /* second moment estimates per param */
} NovaAdamWOptimizer;

/**
 * Create an AdamW optimizer
 * @param lr: learning rate
 * @param beta1: exponential decay for first moments (momentum)
 * @param beta2: exponential decay for second moments (RMS)
 * @param eps: numerical stability epsilon
 * @param weight_decay: decoupled weight decay coefficient
 * @return: allocated optimizer or NULL on failure
 */
NovaAdamWOptimizer *nova_adamw_create(float lr, float beta1, float beta2,
                                      float eps, float weight_decay);

/**
 * Initialize optimizer state (moment buffers)
 * @param opt: optimizer
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 */
void nova_adamw_init_state(NovaAdamWOptimizer *opt, NovaTensor **params, int num_params);

/**
 * Apply one AdamW step to all parameters
 * Assumes parameters have .grad set
 * Features:
 *   - Decoupled weight decay (applied before gradient update)
 *   - Bias correction for early steps
 *   - Adaptive learning rate per parameter
 *
 * @param opt: optimizer
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 */
void nova_adamw_step(NovaAdamWOptimizer *opt, NovaTensor **params, int num_params);

/**
 * Zero out all gradients
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 */
void nova_adamw_zero_grad(NovaTensor **params, int num_params);

/**
 * Free optimizer resources
 * @param opt: optimizer to free
 */
void nova_adamw_free(NovaAdamWOptimizer *opt);

/* ═══════════════════════════════════════════════════════════════════════════
 * Gradient utilities
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Compute L2 norm of all gradients
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 * @return: sqrt(sum of squared gradients)
 */
float nova_get_grad_norm(NovaTensor **params, int num_params);

/**
 * Clip gradients by norm (in-place)
 * If ||grads|| > max_norm, scale all gradients by max_norm / ||grads||
 *
 * @param params: array of parameter tensors
 * @param num_params: number of parameters
 * @param max_norm: maximum allowed gradient norm
 */
void nova_clip_grad_norm(NovaTensor **params, int num_params, float max_norm);

#endif // NOVA_OPTIM_H
