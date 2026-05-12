/**
 * @file autograd.h
 * @brief Internal AI header for automatic differentiation
 */

#ifndef NOVA_AI_autograd_autograd_H
#define NOVA_AI_autograd_autograd_H

#include "ai/tensor.h"
#include "std/collections/vec.h"
#include <stdbool.h>

// Forward declarations
typedef struct nova_grad_node nova_grad_node_t;
typedef struct nova_grad_tape nova_grad_tape_t;
typedef struct nova_grad_fn nova_grad_fn_t;

// ============================================================================
// Operation Types for Gradient Computation
// ============================================================================

typedef enum {
    NOVA_GRAD_OP_ADD,
    NOVA_GRAD_OP_SUB,
    NOVA_GRAD_OP_MUL,
    NOVA_GRAD_OP_DIV,
    NOVA_GRAD_OP_MATMUL,
    NOVA_GRAD_OP_ADD_SCALAR,
    NOVA_GRAD_OP_MUL_SCALAR,
    NOVA_GRAD_OP_EXP,
    NOVA_GRAD_OP_LOG,
    NOVA_GRAD_OP_SQRT,
    NOVA_GRAD_OP_POW,
    NOVA_GRAD_OP_ABS,
    NOVA_GRAD_OP_SIN,
    NOVA_GRAD_OP_COS,
    NOVA_GRAD_OP_TANH,
    NOVA_GRAD_OP_SUM,
    NOVA_GRAD_OP_MEAN,
    NOVA_GRAD_OP_MAX,
    NOVA_GRAD_OP_MIN,
    NOVA_GRAD_OP_CUSTOM,
} nova_grad_op_type_t;

// ============================================================================
// Gradient Function Interface
// ============================================================================

typedef struct nova_grad_fn {
    nova_grad_op_type_t op_type;
    nova_vec_t *inputs;      // input tensors
    nova_vec_t *outputs;     // output tensors
    
    // Gradient computation function
    int (*backward)(nova_grad_fn_t *fn, nova_tensor_t *grad_output);
    
    // Custom data for operations
    void *op_data;
    void (*op_data_cleanup)(void *data);
} nova_grad_fn_t;

// ============================================================================
// Gradient Node (for computation graph)
// ============================================================================

struct nova_grad_node {
    nova_tensor_t *tensor;           // the tensor this node represents
    nova_grad_fn_t *grad_fn;         // function that produced this tensor
    nova_vec_t *inputs;              // input nodes
    bool visited;                    // for topological sort
    int ref_count;                   // reference counting
};

// ============================================================================
// Gradient Tape
// ============================================================================

struct nova_grad_tape {
    nova_vec_t *nodes;               // all nodes in the computation graph
    nova_vec_t *watched_tensors;     // tensors being watched for gradients
    bool recording;                  // whether tape is currently recording
    nova_vec_t *saved_tensors;       // tensors saved for backward pass
};

#endif
