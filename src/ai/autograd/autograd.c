/**
 * @file autograd.c
 * @brief Automatic differentiation implementation
 */

#include "autograd.h"
#include "ai/autograd.h"
#include "std/alloc.h"
#include "std/collections/vec.h"
#include <string.h>

// Global gradient context
static bool g_grad_enabled = true;

// ============================================================================
// Tape Management
// ============================================================================

nova_grad_tape_t *nova_grad_tape_create(void)
{
    nova_grad_tape_t *tape = nova_alloc(sizeof(nova_grad_tape_t));
    if (!tape) return NULL;
    
    tape->nodes = nova_vec_new();
    tape->watched_tensors = nova_vec_new();
    tape->recording = false;
    tape->saved_tensors = nova_vec_new();
    
    return tape;
}

void nova_grad_tape_begin(nova_grad_tape_t *tape)
{
    if (tape) {
        tape->recording = true;
    }
}

void nova_grad_tape_end(nova_grad_tape_t *tape)
{
    if (tape) {
        tape->recording = false;
    }
}

bool nova_grad_tape_is_recording(nova_grad_tape_t *tape)
{
    return tape && tape->recording && g_grad_enabled;
}

void nova_grad_tape_watch(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    if (!tape || !tensor) return;
    
    // Add to watched tensors if not already there
    for (size_t i = 0; i < nova_vec_len(tape->watched_tensors); i++) {
        if (nova_vec_get(tape->watched_tensors, i) == tensor) {
            return; // Already watching
        }
    }
    
    nova_vec_push(tape->watched_tensors, tensor);
    tensor->requires_grad = true;
}

nova_tensor_t *nova_grad_tape_get_grad(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    if (!tape || !tensor) return NULL;
    return tensor->grad;
}

void nova_grad_tape_reset(nova_grad_tape_t *tape)
{
    if (!tape) return;
    
    // Clear all gradients
    for (size_t i = 0; i < nova_vec_len(tape->watched_tensors); i++) {
        nova_tensor_t *tensor = nova_vec_get(tape->watched_tensors, i);
        if (tensor->grad) {
            nova_tensor_destroy(tensor->grad);
            tensor->grad = NULL;
        }
    }
    
    // Clear nodes and saved tensors
    for (size_t i = 0; i < nova_vec_len(tape->nodes); i++) {
        nova_grad_node_t *node = nova_vec_get(tape->nodes, i);
        if (node) {
            // Clean up node resources
            if (node->inputs) nova_vec_destroy(node->inputs);
            nova_free(node);
        }
    }
    nova_vec_clear(tape->nodes);
    nova_vec_clear(tape->saved_tensors);
}

void nova_grad_tape_destroy(nova_grad_tape_t *tape)
{
    if (!tape) return;
    
    nova_grad_tape_reset(tape);
    
    nova_vec_destroy(tape->nodes);
    nova_vec_destroy(tape->watched_tensors);
    nova_vec_destroy(tape->saved_tensors);
    
    nova_free(tape);
}

// ============================================================================
// Gradient Context
// ============================================================================

void nova_no_grad_begin(void)
{
    g_grad_enabled = false;
}

void nova_no_grad_end(void)
{
    g_grad_enabled = true;
}

bool nova_grad_is_enabled(void)
{
    return g_grad_enabled;
}

// ============================================================================
// Tensor Gradient Operations
// ============================================================================

void nova_tensor_set_requires_grad(nova_tensor_t *tensor, bool requires_grad)
{
    if (tensor) {
        tensor->requires_grad = requires_grad;
    }
}

bool nova_tensor_requires_grad(nova_tensor_t *tensor)
{
    return tensor && tensor->requires_grad;
}

nova_tensor_t *nova_tensor_grad(nova_tensor_t *tensor)
{
    return tensor ? tensor->grad : NULL;
}

void nova_tensor_set_grad(nova_tensor_t *tensor, nova_tensor_t *grad)
{
    if (!tensor) return;
    
    if (tensor->grad) {
        nova_tensor_destroy(tensor->grad);
    }
    tensor->grad = grad;
}

void nova_tensor_zero_grad(nova_tensor_t *tensor)
{
    if (!tensor) return;
    
    if (tensor->grad) {
        // Zero out the gradient data
        memset(tensor->grad->data, 0, tensor->grad->size * sizeof(float));
    } else if (tensor->requires_grad) {
        // Create zero gradient
        size_t shape[1] = {tensor->size};
        tensor->grad = nova_tensor_zeros(shape, 1, tensor->dtype);
    }
}

nova_tensor_t *nova_tensor_detach(nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    // Create a new tensor with the same data but no gradient info
    nova_tensor_t *detached = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (detached) {
        memcpy(detached->data, tensor->data, tensor->size * sizeof(float));
        detached->requires_grad = false;
        detached->grad = NULL;
        detached->grad_fn = NULL;
    }
    
    return detached;
}

// ============================================================================
// Higher-Order Gradients
// ============================================================================

/**
 * @brief Compute diagonal of Hessian matrix
 * @details Second derivatives along diagonal: d²L/dx²
 */
nova_tensor_t *nova_grad_hessian_diag(nova_grad_tape_t *tape,
                                      nova_tensor_t *output,
                                      nova_tensor_t *input)
{
    if (!tape || !output || !input) return NULL;
    
    // First compute gradient
    nova_grad_tape_backward(tape, output);
    nova_tensor_t *first_grad = nova_grad_tape_get_grad(tape, input);
    
    if (!first_grad) return NULL;
    
    // Create new tape for second derivative
    nova_grad_tape_t *tape2 = nova_grad_tape_create();
    nova_grad_tape_begin(tape2);
    nova_grad_tape_watch(tape2, first_grad);
    
    // Compute gradient of gradient
    nova_grad_tape_backward(tape2, first_grad);
    nova_tensor_t *hessian_diag = nova_grad_tape_get_grad(tape2, input);
    
    nova_grad_tape_destroy(tape2);
    
    return hessian_diag;
}

/**
 * @brief Vector-Jacobian Product (VJP) - reverse mode
 * @details Computes v^T * J where J is Jacobian
 */
nova_tensor_t *nova_grad_vjp(nova_grad_tape_t *tape,
                             nova_tensor_t *output,
                             nova_tensor_t **inputs,
                             size_t num_inputs,
                             nova_tensor_t *vector)
{
    if (!tape || !output || !inputs || num_inputs == 0 || !vector) return NULL;
    
    // VJP is just weighted backprop
    // Multiply output gradient by vector
    nova_tensor_t *weighted_output = nova_tensor_mul(output, vector);
    
    nova_grad_tape_backward(tape, weighted_output);
    nova_tensor_t *vjp_result = nova_grad_tape_get_grad(tape, inputs[0]);
    
    nova_tensor_destroy(weighted_output);
    
    return vjp_result;
}

/**
 * @brief Compute Jacobian matrix
 * @details Full matrix of all partial derivatives
 */
nova_tensor_t *nova_grad_jacobian(nova_grad_tape_t *tape,
                                  nova_tensor_t **outputs,
                                  size_t num_outputs,
                                  nova_tensor_t **inputs,
                                  size_t num_inputs)
{
    if (!tape || !outputs || num_outputs == 0 || !inputs || num_inputs == 0) return NULL;
    
    nova_tensor_t *output = outputs[0];
    nova_tensor_t *input = inputs[0];
    
    // Jacobian shape: [output_size, input_size]
    size_t jacobian_shape[2] = {output->size, input->size};
    nova_tensor_t *jacobian = nova_tensor_zeros(jacobian_shape, 2, NOVA_DTYPE_FLOAT32);
    
    // Compute each row of Jacobian separately
    for (size_t i = 0; i < output->size; i++) {
        // Create one-hot vector for this output
        size_t onehot_shape[1] = {output->size};
        nova_tensor_t *onehot = nova_tensor_zeros(onehot_shape, 1, NOVA_DTYPE_FLOAT32);
        ((float*)onehot->data)[i] = 1.0f;
        
        // Compute gradient with respect to this output
        nova_grad_tape_reset(tape);
        nova_tensor_t *weighted = nova_tensor_mul(output, onehot);
        nova_grad_tape_backward(tape, weighted);
        nova_tensor_t *grad = nova_grad_tape_get_grad(tape, input);
        
        // Copy gradient to Jacobian row
        if (grad) {
            for (size_t j = 0; j < input->size; j++) {
                ((float*)jacobian->data)[i * input->size + j] = ((float*)grad->data)[j];
            }
        }
        
        nova_tensor_destroy(onehot);
        nova_tensor_destroy(weighted);
    }
    
    return jacobian;
}

/**
 * @brief Compute full Hessian matrix
 * @details Matrix of all second derivatives
 */
nova_tensor_t *nova_grad_hessian_full(nova_grad_tape_t *tape,
                                      nova_tensor_t *output,
                                      nova_tensor_t *input)
{
    if (!tape || !output || !input) return NULL;
    
    // First compute gradient
    nova_grad_tape_backward(tape, output);
    nova_tensor_t *first_grad = nova_grad_tape_get_grad(tape, input);
    
    if (!first_grad) return NULL;
    
    // Compute Jacobian of gradient (which is Hessian)
    nova_grad_tape_t *tape2 = nova_grad_tape_create();
    nova_grad_tape_begin(tape2);
    nova_grad_tape_watch(tape2, first_grad);
    
    nova_tensor_t **outputs = &first_grad;
    nova_tensor_t **inputs = &input;
    nova_tensor_t *hessian = nova_grad_jacobian(tape2, outputs, 1, inputs, 1);
    
    nova_grad_tape_destroy(tape2);
    
    return hessian;
}

/**
 * @brief Jacobian-Vector Product (JVP) - forward mode
 * @details Computes J * v where J is Jacobian
 */
nova_tensor_t *nova_grad_jvp(nova_grad_tape_t *tape,
                             nova_tensor_t *output,
                             nova_tensor_t *input,
                             nova_tensor_t *vector)
{
    if (!tape || !output || !input || !vector) return NULL;
    
    // JVP using finite differences (simplified)
    // In full implementation, would use dual numbers or forward-mode AD
    
    float epsilon = 1e-5f;
    
    // f(x + eps*v)
    nova_tensor_t *perturbed = nova_tensor_add(input, 
        nova_tensor_mul_scalar(vector, epsilon));
    
    // Evaluate function at perturbed point
    // (This would need actual function evaluation - simplified here)
    
    // (f(x + eps*v) - f(x)) / eps approximates J*v
    nova_tensor_t *diff = nova_tensor_sub(output, output); // Placeholder
    nova_tensor_t *jvp_result = nova_tensor_mul_scalar(diff, 1.0f/epsilon);
    
    nova_tensor_destroy(perturbed);
    nova_tensor_destroy(diff);
    
    return jvp_result;
}


int nova_grad_tape_backward(nova_grad_tape_t *tape, nova_tensor_t *loss)
{
    if (!tape || !loss) return -1;
    /* Stub: full backprop via tape nodes would go here */
    (void)tape;
    (void)loss;
    return 0;
}
