/**
 * @file tape.h
 * @brief Internal AI header for gradient tape operations
 */

#ifndef NOVA_AI_autograd_tape_H
#define NOVA_AI_autograd_tape_H

#include "autograd.h"

// ============================================================================
// Tape Management Functions
// ============================================================================

/**
 * Create a new gradient tape
 */
nova_grad_tape_t *nova_grad_tape_create_internal(void);

/**
 * Destroy gradient tape and free all resources
 */
void nova_grad_tape_destroy_internal(nova_grad_tape_t *tape);

/**
 * Start recording operations on the tape
 */
void nova_grad_tape_begin_internal(nova_grad_tape_t *tape);

/**
 * Stop recording operations on the tape
 */
void nova_grad_tape_end_internal(nova_grad_tape_t *tape);

/**
 * Add a tensor to the watch list
 */
void nova_grad_tape_watch_internal(nova_grad_tape_t *tape, nova_tensor_t *tensor);

/**
 * Record an operation on the tape
 */
void nova_grad_tape_record_operation(nova_grad_tape_t *tape,
                                    nova_grad_fn_t *grad_fn,
                                    nova_tensor_t *output);

/**
 * Perform backpropagation from a loss tensor
 */
int nova_grad_tape_backward_internal(nova_grad_tape_t *tape, nova_tensor_t *loss);

/**
 * Get gradient for a watched tensor
 */
nova_tensor_t *nova_grad_tape_get_grad_internal(nova_grad_tape_t *tape, nova_tensor_t *tensor);

/**
 * Reset tape state
 */
void nova_grad_tape_reset_internal(nova_grad_tape_t *tape);

// ============================================================================
// Node Management
// ============================================================================

/**
 * Create a new gradient node
 */
nova_grad_node_t *nova_grad_node_create(nova_tensor_t *tensor, nova_grad_fn_t *grad_fn);

/**
 * Destroy gradient node
 */
void nova_grad_node_destroy(nova_grad_node_t *node);

/**
 * Add input node to a gradient node
 */
void nova_grad_node_add_input(nova_grad_node_t *node, nova_grad_node_t *input);

// ============================================================================
// Gradient Function Management
// ============================================================================

/**
 * Create a gradient function for a specific operation
 */
nova_grad_fn_t *nova_grad_fn_create(nova_grad_op_type_t op_type,
                                   nova_vec_t *inputs,
                                   nova_vec_t *outputs);

/**
 * Destroy gradient function
 */
void nova_grad_fn_destroy(nova_grad_fn_t *grad_fn);

/**
 * Execute backward pass for gradient function
 */
int nova_grad_fn_backward(nova_grad_fn_t *grad_fn, nova_tensor_t *grad_output);

#endif
