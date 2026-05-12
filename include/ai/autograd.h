/**
 * @file autograd.h
 * @brief Automatic differentiation engine
 */

#ifndef NOVA_AUTOGRAD_H
#define NOVA_AUTOGRAD_H

#include "tensor.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Gradient Tape
 * ========================================================================== */

typedef struct nova_grad_tape nova_grad_tape_t;

/**
 * Create gradient tape for recording operations
 * @return Created tape or NULL on error
 */
nova_grad_tape_t *nova_grad_tape_create(void);

/**
 * Start recording operations
 * @param tape The gradient tape
 */
void nova_grad_tape_begin(nova_grad_tape_t *tape);

/**
 * Stop recording operations
 * @param tape The gradient tape
 */
void nova_grad_tape_end(nova_grad_tape_t *tape);

/**
 * Check if tape is currently recording
 * @param tape The gradient tape
 * @return true if recording, false otherwise
 */
bool nova_grad_tape_is_recording(nova_grad_tape_t *tape);

/**
 * Record tensor operation on tape
 * @param tape The gradient tape
 * @param tensor Tensor to watch
 */
void nova_grad_tape_watch(nova_grad_tape_t *tape, nova_tensor_t *tensor);

/**
 * Compute gradients via backpropagation
 * @param tape The gradient tape
 * @param loss Loss tensor (scalar)
 * @return 0 on success, negative on error
 */
int nova_grad_tape_backward(nova_grad_tape_t *tape, nova_tensor_t *loss);

/**
 * Get gradient for a tensor
 * @param tape The gradient tape
 * @param tensor The tensor
 * @return Gradient tensor or NULL if not computed
 */
nova_tensor_t *nova_grad_tape_get_grad(nova_grad_tape_t *tape, nova_tensor_t *tensor);

/**
 * Clear all recorded operations and gradients
 * @param tape The gradient tape
 */
void nova_grad_tape_reset(nova_grad_tape_t *tape);

/**
 * Destroy gradient tape
 * @param tape The tape to destroy
 */
void nova_grad_tape_destroy(nova_grad_tape_t *tape);

/* ============================================================================
 * Gradient Context
 * ========================================================================== */

/**
 * Disable gradient computation (inference mode)
 */
void nova_no_grad_begin(void);

/**
 * Re-enable gradient computation
 */
void nova_no_grad_end(void);

/**
 * Check if gradients are enabled
 * @return true if enabled, false otherwise
 */
bool nova_grad_is_enabled(void);

/* ============================================================================
 * Tensor Gradient Operations
 * ========================================================================== */

/**
 * Mark tensor as requiring gradients
 * @param tensor The tensor
 * @param requires_grad Whether to require gradients
 */
void nova_tensor_set_requires_grad(nova_tensor_t *tensor, bool requires_grad);

/**
 * Check if tensor requires gradients
 * @param tensor The tensor
 * @return true if requires gradients, false otherwise
 */
bool nova_tensor_requires_grad(nova_tensor_t *tensor);

/**
 * Get gradient of tensor
 * @param tensor The tensor
 * @return Gradient tensor or NULL if not computed
 */
nova_tensor_t *nova_tensor_grad(nova_tensor_t *tensor);

/**
 * Set gradient of tensor
 * @param tensor The tensor
 * @param grad Gradient tensor
 */
void nova_tensor_set_grad(nova_tensor_t *tensor, nova_tensor_t *grad);

/**
 * Zero out gradients
 * @param tensor The tensor
 */
void nova_tensor_zero_grad(nova_tensor_t *tensor);

/**
 * Detach tensor from computation graph
 * @param tensor The tensor
 * @return Detached tensor (shares data but no gradient)
 */
nova_tensor_t *nova_tensor_detach(nova_tensor_t *tensor);

/* ============================================================================
 * Custom Gradient Functions
 * ========================================================================== */

typedef struct nova_grad_fn nova_grad_fn_t;

typedef struct {
    nova_tensor_t *(*forward)(void *ctx, nova_tensor_t **inputs, size_t num_inputs);
    nova_tensor_t **(*backward)(void *ctx, nova_tensor_t *grad_output, size_t *num_grads);
    void (*cleanup)(void *ctx);
} nova_custom_grad_ops_t;

/**
 * Create custom gradient function
 * @param ops Operations for forward/backward pass
 * @param ctx User context
 * @return Created gradient function or NULL on error
 */
nova_grad_fn_t *nova_custom_grad_create(const nova_custom_grad_ops_t *ops, void *ctx);

/**
 * Apply custom gradient function
 * @param grad_fn The gradient function
 * @param inputs Input tensors
 * @param num_inputs Number of inputs
 * @return Output tensor
 */
nova_tensor_t *nova_custom_grad_apply(nova_grad_fn_t *grad_fn,
                                     nova_tensor_t **inputs,
                                     size_t num_inputs);

/* ============================================================================
 * Higher-Order Gradients
 * ========================================================================== */

/**
 * Compute second-order gradients (Hessian diagonal)
 * @param tape The gradient tape
 * @param loss Loss tensor
 * @param tensor Tensor to compute Hessian for
 * @return Hessian diagonal tensor
 */
nova_tensor_t *nova_grad_hessian_diag(nova_grad_tape_t *tape,
                                     nova_tensor_t *loss,
                                     nova_tensor_t *tensor);

/**
 * Compute gradient-vector product
 * @param tape The gradient tape
 * @param output Output tensor
 * @param inputs Input tensors
 * @param num_inputs Number of inputs
 * @param vector Vector to multiply with
 * @return Gradient-vector product
 */
nova_tensor_t *nova_grad_vjp(nova_grad_tape_t *tape,
                            nova_tensor_t *output,
                            nova_tensor_t **inputs,
                            size_t num_inputs,
                            nova_tensor_t *vector);

/**
 * Compute Jacobian matrix
 * @param tape The gradient tape
 * @param outputs Output tensors
 * @param num_outputs Number of outputs
 * @param inputs Input tensors
 * @param num_inputs Number of inputs
 * @return Jacobian matrix tensor
 */
nova_tensor_t *nova_grad_jacobian(nova_grad_tape_t *tape,
                                 nova_tensor_t **outputs,
                                 size_t num_outputs,
                                 nova_tensor_t **inputs,
                                 size_t num_inputs);

/* ============================================================================
 * Gradient Clipping
 * ========================================================================== */

/**
 * Clip gradients by value
 * @param tensors Array of tensors
 * @param num_tensors Number of tensors
 * @param min_value Minimum value
 * @param max_value Maximum value
 */
void nova_grad_clip_value(nova_tensor_t **tensors, size_t num_tensors,
                         float min_value, float max_value);

/**
 * Clip gradients by global norm
 * @param tensors Array of tensors
 * @param num_tensors Number of tensors
 * @param max_norm Maximum norm
 * @return Computed global norm before clipping
 */
float nova_grad_clip_norm(nova_tensor_t **tensors, size_t num_tensors,
                         float max_norm);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_AUTOGRAD_H */

/**
 * @brief Compute full Hessian matrix (all second derivatives)
 * @param tape Gradient tape
 * @param output Output tensor (scalar)
 * @param input Input tensor
 * @return Hessian matrix [input_size x input_size]
 */
nova_tensor_t *nova_grad_hessian(nova_grad_tape_t *tape,
                                 nova_tensor_t *output,
                                 nova_tensor_t *input);

/**
 * @brief Jacobian-Vector Product (forward-mode)
 * @param tape Gradient tape
 * @param output Output tensor
 * @param input Input tensor
 * @param vector Direction vector
 * @return JVP result
 */
nova_tensor_t *nova_grad_jvp(nova_grad_tape_t *tape,
                            nova_tensor_t *output,
                            nova_tensor_t *input,
                            nova_tensor_t *vector);

