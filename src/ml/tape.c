// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA GRADIENT TAPE SYSTEM  v2.0                                       ║
// ║  Automatic differentiation and gradient computation                     ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#include "ml/tape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// GRADIENT TAPE
// ══════════════════════════════════════════════════════════════════════════════

struct nova_grad_tape {
    nova_grad_node_t **nodes;
    size_t node_count;
    size_t node_capacity;
    bool recording;
};

struct nova_grad_node {
    nova_tensor_t *tensor;
    nova_grad_fn_t *grad_fn;
    nova_tensor_t *grad;
    size_t ref_count;
};

// ══════════════════════════════════════════════════════════════════════════════
// TAPE LIFECYCLE
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_tape_t *nova_grad_tape_create(void)
{
    nova_grad_tape_t *tape = calloc(1, sizeof(nova_grad_tape_t));
    if (!tape)
        return NULL;

    tape->node_capacity = 1024;
    tape->nodes = calloc(tape->node_capacity, sizeof(nova_grad_node_t *));
    if (!tape->nodes) {
        free(tape);
        return NULL;
    }

    tape->node_count = 0;
    tape->recording = true;

    return tape;
}

void nova_grad_tape_destroy(nova_grad_tape_t *tape)
{
    if (!tape)
        return;

    for (size_t i = 0; i < tape->node_count; i++) {
        nova_grad_node_destroy(tape->nodes[i]);
    }

    free(tape->nodes);
    free(tape);
}

// ══════════════════════════════════════════════════════════════════════════════
// NODE MANAGEMENT
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_node_t *nova_grad_node_create(nova_tensor_t *tensor, nova_grad_fn_t *grad_fn)
{
    nova_grad_node_t *node = calloc(1, sizeof(nova_grad_node_t));
    if (!node)
        return NULL;

    node->tensor = nova_tensor_clone(tensor);
    node->grad_fn = grad_fn; // Take ownership
    node->grad = NULL;
    node->ref_count = 1;

    return node;
}

void nova_grad_node_destroy(nova_grad_node_t *node)
{
    if (!node)
        return;

    node->ref_count--;
    if (node->ref_count > 0)
        return;

    nova_tensor_destroy(node->tensor);
    nova_grad_fn_destroy(node->grad_fn);
    nova_tensor_destroy(node->grad);
    free(node);
}

int nova_grad_tape_add_node(nova_grad_tape_t *tape, nova_grad_node_t *node)
{
    if (!tape || !tape->recording)
        return 0;

    if (tape->node_count >= tape->node_capacity) {
        tape->node_capacity *= 2;
        tape->nodes = realloc(tape->nodes, sizeof(nova_grad_node_t *) * tape->node_capacity);
        if (!tape->nodes)
            return -1;
    }

    tape->nodes[tape->node_count++] = node;
    node->ref_count++; // Tape holds a reference

    return 0;
}

// ══════════════════════════════════════════════════════════════════════════════
// RECORDING OPERATIONS
// ══════════════════════════════════════════════════════════════════════════════

int nova_grad_tape_record_operation(nova_grad_tape_t *tape, nova_tensor_t *result,
                                    nova_grad_fn_t *grad_fn)
{
    if (!tape || !tape->recording || !result)
        return 0;

    nova_grad_node_t *node = nova_grad_node_create(result, grad_fn);
    if (!node)
        return -1;

    result->grad_fn = grad_fn; // Link to tensor

    return nova_grad_tape_add_node(tape, node);
}

// ══════════════════════════════════════════════════════════════════════════════
// BACKWARD PASS
// ══════════════════════════════════════════════════════════════════════════════

int nova_grad_tape_backward(nova_grad_tape_t *tape, nova_tensor_t *output)
{
    if (!tape || !output)
        return -1;

    // Find the output node
    nova_grad_node_t *output_node = NULL;
    for (size_t i = 0; i < tape->node_count; i++) {
        if (tape->nodes[i]->tensor == output) {
            output_node = tape->nodes[i];
            break;
        }
    }

    if (!output_node)
        return -1;

    // Create gradient output (ones tensor with same shape)
    nova_tensor_t *grad_output = nova_tensor_ones(output->dtype, output->ndims, output->shape);
    if (!grad_output)
        return -1;

    // Start backward pass
    int result = nova_grad_tape_backward_with_grad(tape, output_node, grad_output);
    nova_tensor_destroy(grad_output);

    return result;
}

int nova_grad_tape_backward_with_grad(nova_grad_tape_t *tape, nova_grad_node_t *node,
                                      nova_tensor_t *grad_output)
{
    if (!node || !grad_output)
        return -1;

    // Store gradient
    node->grad = nova_tensor_clone(grad_output);

    // If no gradient function, stop
    if (!node->grad_fn)
        return 0;

    // Compute gradients for inputs
    nova_tensor_t *grad_input1 = NULL;
    nova_tensor_t *grad_input2 = NULL;

    node->grad_fn->backward(node->grad_fn, grad_output, &grad_input1, &grad_input2);

    // Find and propagate to input nodes
    if (node->grad_fn->input1 && grad_input1) {
        nova_grad_node_t *input_node = nova_grad_tape_find_node(tape, node->grad_fn->input1);
        if (input_node) {
            nova_grad_tape_backward_with_grad(tape, input_node, grad_input1);
        }
        nova_tensor_destroy(grad_input1);
    }

    if (node->grad_fn->input2 && grad_input2) {
        nova_grad_node_t *input_node = nova_grad_tape_find_node(tape, node->grad_fn->input2);
        if (input_node) {
            nova_grad_tape_backward_with_grad(tape, input_node, grad_input2);
        }
        nova_tensor_destroy(grad_input2);
    }

    return 0;
}

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_node_t *nova_grad_tape_find_node(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    if (!tape || !tensor)
        return NULL;

    for (size_t i = 0; i < tape->node_count; i++) {
        if (tape->nodes[i]->tensor == tensor) {
            return tape->nodes[i];
        }
    }

    return NULL;
}

void nova_grad_tape_start_recording(nova_grad_tape_t *tape)
{
    if (tape)
        tape->recording = true;
}

void nova_grad_tape_stop_recording(nova_grad_tape_t *tape)
{
    if (tape)
        tape->recording = false;
}

bool nova_grad_tape_is_recording(nova_grad_tape_t *tape)
{
    return tape && tape->recording;
}

size_t nova_grad_tape_node_count(nova_grad_tape_t *tape)
{
    return tape ? tape->node_count : 0;
}

// ══════════════════════════════════════════════════════════════════════════════
// GRADIENT FUNCTION IMPLEMENTATIONS
// ══════════════════════════════════════════════════════════════════════════════

static void add_backward_impl(nova_grad_fn_t *fn, nova_tensor_t *grad_output,
                              nova_tensor_t **grad_input1, nova_tensor_t **grad_input2)
{
    // d(a+b)/da = 1, d(a+b)/db = 1
    *grad_input1 = nova_tensor_clone(grad_output);
    *grad_input2 = nova_tensor_clone(grad_output);
}

static void matmul_backward_impl(nova_grad_fn_t *fn, nova_tensor_t *grad_output,
                                 nova_tensor_t **grad_input1, nova_tensor_t **grad_input2)
{
    // d(A@B)/dA = grad_output @ B^T
    nova_tensor_t *bt = nova_tensor_transpose(fn->input2);
    *grad_input1 = nova_tensor_matmul(grad_output, bt);
    nova_tensor_destroy(bt);

    // d(A@B)/dB = A^T @ grad_output
    nova_tensor_t *at = nova_tensor_transpose(fn->input1);
    *grad_input2 = nova_tensor_matmul(at, grad_output);
    nova_tensor_destroy(at);
}

nova_grad_fn_t *nova_grad_fn_create_add(const nova_tensor_t *a, const nova_tensor_t *b)
{
    nova_grad_fn_t *fn = calloc(1, sizeof(nova_grad_fn_t));
    if (!fn)
        return NULL;

    fn->type = NOVA_GRAD_FN_ADD;
    fn->input1 = nova_tensor_clone(a);
    fn->input2 = nova_tensor_clone(b);
    fn->backward = add_backward_impl;

    return fn;
}

nova_grad_fn_t *nova_grad_fn_create_matmul(const nova_tensor_t *a, const nova_tensor_t *b)
{
    nova_grad_fn_t *fn = calloc(1, sizeof(nova_grad_fn_t));
    if (!fn)
        return NULL;

    fn->type = NOVA_GRAD_FN_MATMUL;
    fn->input1 = nova_tensor_clone(a);
    fn->input2 = nova_tensor_clone(b);
    fn->backward = matmul_backward_impl;

    return fn;
}

void nova_grad_fn_destroy(nova_grad_fn_t *fn)
{
    if (!fn)
        return;

    nova_tensor_destroy(fn->input1);
    nova_tensor_destroy(fn->input2);
    free(fn);
}

// ══════════════════════════════════════════════════════════════════════════════
// HIGH-LEVEL API
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_tape_t *nova_grad_tape_new(void)
{
    return nova_grad_tape_create();
}

void nova_grad_tape_delete(nova_grad_tape_t *tape)
{
    nova_grad_tape_destroy(tape);
}

int nova_grad_tape_watch(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    if (!tape || !tensor || !tape->recording)
        return 0;

    nova_grad_node_t *node = nova_grad_node_create(tensor, NULL);
    if (!node)
        return -1;

    return nova_grad_tape_add_node(tape, node);
}

nova_tensor_t *nova_grad_tape_gradient(nova_grad_tape_t *tape, nova_tensor_t *tensor)
{
    nova_grad_node_t *node = nova_grad_tape_find_node(tape, tensor);
    return node ? nova_tensor_clone(node->grad) : NULL;
}
