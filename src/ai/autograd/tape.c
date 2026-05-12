/**
 * @file tape.c
 * @brief Gradient tape implementation
 */

#include "tape.h"
#include "autograd.h"
#include "ai/autograd.h"
#include "std/alloc.h"
#include "std/collections/vec.h"
#include <string.h>
#include <math.h>

/* Forward declarations for backward functions defined later in this file */
static int nova_grad_fn_backward_add(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_sub(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_mul(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_div(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_add_scalar(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_mul_scalar(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_exp(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_log(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_sqrt(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_pow(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_abs(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_sin(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_cos(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_tanh(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_relu(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_sigmoid(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_matmul(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_sum(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_mean(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_reshape(nova_grad_fn_t *fn, nova_tensor_t *grad);
static int nova_grad_fn_backward_transpose(nova_grad_fn_t *fn, nova_tensor_t *grad);

// ============================================================================
// Internal Tape Management
// ============================================================================

nova_grad_tape_t *nova_grad_tape_create_internal(void)
{
    return nova_grad_tape_create();
}

void nova_grad_tape_destroy_internal(nova_grad_tape_t *tape)
{
    nova_grad_tape_destroy(tape);
}

void nova_grad_tape_begin_internal(nova_grad_tape_t *tape)
{
    nova_grad_tape_begin(tape);
}

void nova_grad_tape_end_internal(nova_grad_tape_t *tape)
{
    nova_grad_tape_end(tape);
}

void nova_grad_tape_watch_internal(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    nova_grad_tape_watch(tape, tensor);
}

void nova_grad_tape_reset_internal(nova_grad_tape_t *tape)
{
    nova_grad_tape_reset(tape);
}

nova_tensor_t *nova_grad_tape_get_grad_internal(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    return nova_grad_tape_get_grad(tape, tensor);
}

// ============================================================================
// Backpropagation Implementation
// ============================================================================

static void topological_sort_visit(nova_grad_node_t *node, nova_vec_t *sorted)
{
    if (node->visited) return;
    node->visited = true;
    
    // Visit all inputs first
    for (size_t i = 0; i < nova_vec_len(node->inputs); i++) {
        nova_grad_node_t *input = nova_vec_get(node->inputs, i);
        topological_sort_visit(input, sorted);
    }
    
    // Add this node to sorted list
    nova_vec_push(sorted, node);
}

static nova_vec_t *topological_sort(nova_grad_tape_t *tape)
{
    nova_vec_t *sorted = nova_vec_new();
    
    // Reset visited flags
    for (size_t i = 0; i < nova_vec_len(tape->nodes); i++) {
        nova_grad_node_t *node = nova_vec_get(tape->nodes, i);
        node->visited = false;
    }
    
    // Perform topological sort starting from output nodes
    for (size_t i = 0; i < nova_vec_len(tape->nodes); i++) {
        nova_grad_node_t *node = nova_vec_get(tape->nodes, i);
        // Only start from nodes that have no outputs (leaf nodes in reverse)
        bool is_output = true;
        for (size_t j = 0; j < nova_vec_len(tape->nodes); j++) {
            nova_grad_node_t *other = nova_vec_get(tape->nodes, j);
            for (size_t k = 0; k < nova_vec_len(other->inputs); k++) {
                if (nova_vec_get(other->inputs, k) == node) {
                    is_output = false;
                    break;
                }
            }
            if (!is_output) break;
        }
        if (is_output) {
            topological_sort_visit(node, sorted);
        }
    }
    
    return sorted;
}

int nova_grad_tape_backward_internal(nova_grad_tape_t *tape, nova_tensor_t *loss)
{
    if (!tape || !loss) return -1;
    
    // Create gradient for loss (initially ones)
    size_t loss_shape[1] = {loss->size};
    nova_tensor_t *loss_grad = nova_tensor_ones(loss_shape, 1, loss->dtype);
    
    // Find the node corresponding to loss
    nova_grad_node_t *loss_node = NULL;
    for (size_t i = 0; i < nova_vec_len(tape->nodes); i++) {
        nova_grad_node_t *node = nova_vec_get(tape->nodes, i);
        if (node->tensor == loss) {
            loss_node = node;
            break;
        }
    }
    
    if (!loss_node) {
        nova_tensor_destroy(loss_grad);
        return -1; // Loss not found in tape
    }
    
    // Perform reverse-mode autodiff
    // For simplicity, we'll implement a basic version
    // In a full implementation, we'd do topological sort and accumulate gradients
    
    // Set gradient for loss
    nova_tensor_set_grad(loss, loss_grad);
    
    // Propagate gradients backward
    // This is a simplified implementation - real backprop would be more complex
    for (size_t i = nova_vec_len(tape->nodes); i > 0; i--) {
        nova_grad_node_t *node = nova_vec_get(tape->nodes, i - 1);
        if (node->tensor->grad && node->grad_fn) {
            // Call backward on the gradient function
            nova_grad_fn_backward(node->grad_fn, node->tensor->grad);
        }
    }
    
    return 0;
}

// ============================================================================
// Node Management
// ============================================================================

nova_grad_node_t *nova_grad_node_create(nova_tensor_t *tensor, nova_grad_fn_t *grad_fn)
{
    nova_grad_node_t *node = nova_alloc(sizeof(nova_grad_node_t));
    if (!node) return NULL;
    
    node->tensor = tensor;
    node->grad_fn = grad_fn;
    node->inputs = nova_vec_new();
    node->visited = false;
    node->ref_count = 1;
    
    return node;
}

void nova_grad_node_destroy(nova_grad_node_t *node)
{
    if (!node) return;
    
    if (--node->ref_count > 0) return;
    
    nova_vec_destroy(node->inputs);
    if (node->grad_fn) {
        nova_grad_fn_destroy(node->grad_fn);
    }
    
    nova_free(node);
}

void nova_grad_node_add_input(nova_grad_node_t *node, nova_grad_node_t *input)
{
    if (node && input) {
        nova_vec_push(node->inputs, input);
        input->ref_count++;
    }
}

// ============================================================================
// Gradient Function Management
// ============================================================================

nova_grad_fn_t *nova_grad_fn_create(nova_grad_op_type_t op_type,
                                   nova_vec_t *inputs,
                                   nova_vec_t *outputs)
{
    nova_grad_fn_t *grad_fn = nova_alloc(sizeof(nova_grad_fn_t));
    if (!grad_fn) return NULL;
    
    grad_fn->op_type = op_type;
    grad_fn->inputs = nova_vec_new();
    grad_fn->outputs = nova_vec_new();
    
    // Copy inputs
    if (inputs) {
        for (size_t i = 0; i < nova_vec_len(inputs); i++) {
            nova_vec_push(grad_fn->inputs, nova_vec_get(inputs, i));
        }
    }
    
    // Copy outputs
    if (outputs) {
        for (size_t i = 0; i < nova_vec_len(outputs); i++) {
            nova_vec_push(grad_fn->outputs, nova_vec_get(outputs, i));
        }
    }
    
    grad_fn->backward = NULL; // Will be set based on operation type
    grad_fn->op_data = NULL;
    grad_fn->op_data_cleanup = NULL;
    
    // Set backward function based on operation type
    switch (op_type) {
        case NOVA_GRAD_OP_ADD:
            grad_fn->backward = nova_grad_fn_backward_add;
            break;
        case NOVA_GRAD_OP_SUB:
            grad_fn->backward = nova_grad_fn_backward_sub;
            break;
        case NOVA_GRAD_OP_MUL:
            grad_fn->backward = nova_grad_fn_backward_mul;
            break;
        case NOVA_GRAD_OP_DIV:
            grad_fn->backward = nova_grad_fn_backward_div;
            break;
        case NOVA_GRAD_OP_ADD_SCALAR:
            grad_fn->backward = nova_grad_fn_backward_add_scalar;
            break;
        case NOVA_GRAD_OP_MUL_SCALAR:
            grad_fn->backward = nova_grad_fn_backward_mul_scalar;
            break;
        case NOVA_GRAD_OP_EXP:
            grad_fn->backward = nova_grad_fn_backward_exp;
            break;
        case NOVA_GRAD_OP_LOG:
            grad_fn->backward = nova_grad_fn_backward_log;
            break;
        case NOVA_GRAD_OP_SQRT:
            grad_fn->backward = nova_grad_fn_backward_sqrt;
            break;
        case NOVA_GRAD_OP_POW:
            grad_fn->backward = nova_grad_fn_backward_pow;
            break;
        case NOVA_GRAD_OP_ABS:
            grad_fn->backward = nova_grad_fn_backward_abs;
            break;
        case NOVA_GRAD_OP_SIN:
            grad_fn->backward = nova_grad_fn_backward_sin;
            break;
        case NOVA_GRAD_OP_COS:
            grad_fn->backward = nova_grad_fn_backward_cos;
            break;
        case NOVA_GRAD_OP_TANH:
            grad_fn->backward = nova_grad_fn_backward_tanh;
            break;
        // Add more cases as needed
        default:
            break;
    }
    
    return grad_fn;
}

void nova_grad_fn_destroy(nova_grad_fn_t *grad_fn)
{
    if (!grad_fn) return;
    
    nova_vec_destroy(grad_fn->inputs);
    nova_vec_destroy(grad_fn->outputs);
    
    if (grad_fn->op_data && grad_fn->op_data_cleanup) {
        grad_fn->op_data_cleanup(grad_fn->op_data);
    }
    
    nova_free(grad_fn);
}

int nova_grad_fn_backward(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    if (!grad_fn || !grad_fn->backward) return -1;
    return grad_fn->backward(grad_fn, grad_output);
}

// ============================================================================
// Operation-Specific Backward Functions
// ============================================================================

static int nova_grad_fn_backward_add(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For addition: gradient flows equally to both inputs
    if (nova_vec_len(grad_fn->inputs) >= 2) {
        nova_tensor_t *input1 = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *input2 = nova_vec_get(grad_fn->inputs, 1);
        
        if (input1->requires_grad) {
            if (!input1->grad) {
                size_t shape[1] = {input1->size};
                input1->grad = nova_tensor_zeros(shape, 1, input1->dtype);
            }
            // Accumulate gradient
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input1->grad->data)[i] += ((float*)grad_output->data)[i];
            }
        }
        
        if (input2->requires_grad) {
            if (!input2->grad) {
                size_t shape[1] = {input2->size};
                input2->grad = nova_tensor_zeros(shape, 1, input2->dtype);
            }
            // Accumulate gradient
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input2->grad->data)[i] += ((float*)grad_output->data)[i];
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_mul(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For multiplication: d/dx(xy) = y, d/dy(xy) = x
    if (nova_vec_len(grad_fn->inputs) >= 2) {
        nova_tensor_t *input1 = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *input2 = nova_vec_get(grad_fn->inputs, 1);
        
        if (input1->requires_grad) {
            if (!input1->grad) {
                size_t shape[1] = {input1->size};
                input1->grad = nova_tensor_zeros(shape, 1, input1->dtype);
            }
            // grad_input1 = grad_output * input2
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input1->grad->data)[i] += ((float*)grad_output->data)[i] * ((float*)input2->data)[i];
            }
        }
        
        if (input2->requires_grad) {
            if (!input2->grad) {
                size_t shape[1] = {input2->size};
                input2->grad = nova_tensor_zeros(shape, 1, input2->dtype);
            }
            // grad_input2 = grad_output * input1
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input2->grad->data)[i] += ((float*)grad_output->data)[i] * ((float*)input1->data)[i];
            }
        }
    }
    
    return 0;
}

// ============================================================================
// Additional Backward Functions
// ============================================================================

static int nova_grad_fn_backward_sub(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For subtraction: d/dx(x-y) = 1, d/dy(x-y) = -1
    if (nova_vec_len(grad_fn->inputs) >= 2) {
        nova_tensor_t *input1 = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *input2 = nova_vec_get(grad_fn->inputs, 1);
        
        if (input1->requires_grad) {
            if (!input1->grad) {
                size_t shape[1] = {input1->size};
                input1->grad = nova_tensor_zeros(shape, 1, input1->dtype);
            }
            // Accumulate gradient (+1 * grad_output)
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input1->grad->data)[i] += ((float*)grad_output->data)[i];
            }
        }
        
        if (input2->requires_grad) {
            if (!input2->grad) {
                size_t shape[1] = {input2->size};
                input2->grad = nova_tensor_zeros(shape, 1, input2->dtype);
            }
            // Accumulate gradient (-1 * grad_output)
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input2->grad->data)[i] -= ((float*)grad_output->data)[i];
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_div(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For division: d/dx(x/y) = 1/y, d/dy(x/y) = -x/y^2
    if (nova_vec_len(grad_fn->inputs) >= 2) {
        nova_tensor_t *input1 = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *input2 = nova_vec_get(grad_fn->inputs, 1);
        
        if (input1->requires_grad) {
            if (!input1->grad) {
                size_t shape[1] = {input1->size};
                input1->grad = nova_tensor_zeros(shape, 1, input1->dtype);
            }
            // grad_input1 = grad_output / input2
            for (size_t i = 0; i < grad_output->size; i++) {
                float y = ((float*)input2->data)[i];
                if (y != 0.0f) {
                    ((float*)input1->grad->data)[i] += ((float*)grad_output->data)[i] / y;
                }
            }
        }
        
        if (input2->requires_grad) {
            if (!input2->grad) {
                size_t shape[1] = {input2->size};
                input2->grad = nova_tensor_zeros(shape, 1, input2->dtype);
            }
            // grad_input2 = -grad_output * input1 / (input2^2)
            for (size_t i = 0; i < grad_output->size; i++) {
                float x = ((float*)input1->data)[i];
                float y = ((float*)input2->data)[i];
                if (y != 0.0f) {
                    ((float*)input2->grad->data)[i] -= ((float*)grad_output->data)[i] * x / (y * y);
                }
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_add_scalar(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For scalar addition: d/dx(x + c) = 1
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // Accumulate gradient (grad_output flows through unchanged)
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i];
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_mul_scalar(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For scalar multiplication: d/dx(x * c) = c
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // Get the scalar from op_data
            float scalar = *(float*)grad_fn->op_data;
            // grad_input = grad_output * scalar
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] * scalar;
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_exp(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For exp: d/dx(exp(x)) = exp(x)
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *output = nova_vec_get(grad_fn->outputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output * exp(input)
            for (size_t i = 0; i < grad_output->size; i++) {
                ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] * ((float*)output->data)[i];
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_log(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For log: d/dx(log(x)) = 1/x
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output / input
            for (size_t i = 0; i < grad_output->size; i++) {
                float x = ((float*)input->data)[i];
                if (x != 0.0f) {
                    ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] / x;
                }
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_sqrt(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For sqrt: d/dx(sqrt(x)) = 1/(2*sqrt(x))
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *output = nova_vec_get(grad_fn->outputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output / (2 * sqrt(input))
            for (size_t i = 0; i < grad_output->size; i++) {
                float sqrt_x = ((float*)output->data)[i];
                if (sqrt_x != 0.0f) {
                    ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] / (2.0f * sqrt_x);
                }
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_pow(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For pow: d/dx(x^y) = y * x^(y-1)
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // Get exponent from op_data
            float exponent = *(float*)grad_fn->op_data;
            // grad_input = grad_output * exponent * input^(exponent-1)
            for (size_t i = 0; i < grad_output->size; i++) {
                float x = ((float*)input->data)[i];
                float grad = ((float*)grad_output->data)[i] * exponent * powf(x, exponent - 1.0f);
                ((float*)input->grad->data)[i] += grad;
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_abs(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For abs: d/dx(|x|) = sign(x)
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output * sign(input)
            for (size_t i = 0; i < grad_output->size; i++) {
                float x = ((float*)input->data)[i];
                float sign = (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f);
                ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] * sign;
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_sin(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For sin: d/dx(sin(x)) = cos(x)
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output * cos(input)
            for (size_t i = 0; i < grad_output->size; i++) {
                float x = ((float*)input->data)[i];
                ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] * cosf(x);
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_cos(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For cos: d/dx(cos(x)) = -sin(x)
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output * (-sin(input))
            for (size_t i = 0; i < grad_output->size; i++) {
                float x = ((float*)input->data)[i];
                ((float*)input->grad->data)[i] += ((float*)grad_output->data)[i] * (-sinf(x));
            }
        }
    }
    
    return 0;
}

static int nova_grad_fn_backward_tanh(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output)
{
    // For tanh: d/dx(tanh(x)) = 1 - tanh(x)^2
    if (nova_vec_len(grad_fn->inputs) >= 1) {
        nova_tensor_t *input = nova_vec_get(grad_fn->inputs, 0);
        nova_tensor_t *output = nova_vec_get(grad_fn->outputs, 0);
        
        if (input->requires_grad) {
            if (!input->grad) {
                size_t shape[1] = {input->size};
                input->grad = nova_tensor_zeros(shape, 1, input->dtype);
            }
            // grad_input = grad_output * (1 - tanh(input)^2)
            for (size_t i = 0; i < grad_output->size; i++) {
                float tanh_x = ((float*)output->data)[i];
                float grad = ((float*)grad_output->data)[i] * (1.0f - tanh_x * tanh_x);
                ((float*)input->grad->data)[i] += grad;
            }
        }
    }
    
    return 0;
}

// ============================================================================
// Tape Recording
// ============================================================================

void nova_grad_tape_record_operation(nova_grad_tape_t *tape,
                                    nova_grad_fn_t *grad_fn,
                                    nova_tensor_t *output)
{
    if (!tape || !nova_grad_tape_is_recording(tape) || !output) return;
    
    // Create node for this operation
    nova_grad_node_t *node = nova_grad_node_create(output, grad_fn);
    if (node) {
        nova_vec_push(tape->nodes, node);
        
        // Add input nodes
        for (size_t i = 0; i < nova_vec_len(grad_fn->inputs); i++) {
            nova_tensor_t *input = nova_vec_get(grad_fn->inputs, i);
            // Find or create input node
            nova_grad_node_t *input_node = NULL;
            for (size_t j = 0; j < nova_vec_len(tape->nodes); j++) {
                nova_grad_node_t *existing = nova_vec_get(tape->nodes, j);
                if (existing->tensor == input) {
                    input_node = existing;
                    break;
                }
            }
            if (!input_node) {
                input_node = nova_grad_node_create(input, NULL);
                nova_vec_push(tape->nodes, input_node);
            }
            nova_grad_node_add_input(node, input_node);
        }
    }
}
