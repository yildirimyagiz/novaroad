// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA GRADIENT TAPE SYSTEM  v2.0                                       ║
// ║  Automatic differentiation and gradient computation                     ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_ML_TAPE_H
#define NOVA_ML_TAPE_H

#include "tensor.h"
#include <stdbool.h>
#include <stddef.h>

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

typedef struct nova_grad_tape nova_grad_tape_t;
typedef struct nova_grad_node nova_grad_node_t;
typedef struct nova_tensor nova_tensor_t;
typedef struct nova_grad_fn nova_grad_fn_t;

// ══════════════════════════════════════════════════════════════════════════════
// TAPE LIFECYCLE
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_tape_t *nova_grad_tape_create(void);
void nova_grad_tape_destroy(nova_grad_tape_t *tape);
nova_grad_tape_t *nova_grad_tape_new(void);         // Alias
void nova_grad_tape_delete(nova_grad_tape_t *tape); // Alias

// ══════════════════════════════════════════════════════════════════════════════
// RECORDING CONTROL
// ══════════════════════════════════════════════════════════════════════════════

void nova_grad_tape_start_recording(nova_grad_tape_t *tape);
void nova_grad_tape_stop_recording(nova_grad_tape_t *tape);
bool nova_grad_tape_is_recording(nova_grad_tape_t *tape);
int nova_grad_tape_watch(nova_grad_tape_t *tape, nova_tensor_t *tensor);

// ══════════════════════════════════════════════════════════════════════════════
// GRADIENT COMPUTATION
// ══════════════════════════════════════════════════════════════════════════════

/// Run backward pass from the output tensor
int nova_grad_tape_backward(nova_grad_tape_t *tape, nova_tensor_t *output);

/// Internal backward pass with explicit gradient
int nova_grad_tape_backward_with_grad(nova_grad_tape_t *tape, nova_grad_node_t *node,
                                      nova_tensor_t *grad_output);

/// Get computed gradient for a specific tensor
nova_tensor_t *nova_grad_tape_gradient(nova_grad_tape_t *tape, nova_tensor_t *tensor);

// ══════════════════════════════════════════════════════════════════════════════
// NODE MANAGEMENT (Internal)
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_node_t *nova_grad_node_create(nova_tensor_t *tensor, nova_grad_fn_t *grad_fn);
void nova_grad_node_destroy(nova_grad_node_t *node);
int nova_grad_tape_add_node(nova_grad_tape_t *tape, nova_grad_node_t *node);
nova_grad_node_t *nova_grad_tape_find_node(nova_grad_tape_t *tape, nova_tensor_t *tensor);
size_t nova_grad_tape_node_count(nova_grad_tape_t *tape);

// ══════════════════════════════════════════════════════════════════════════════
// GRADIENT FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_fn_t *nova_grad_fn_create_add(const nova_tensor_t *left, const nova_tensor_t *right);
nova_grad_fn_t *nova_grad_fn_create_matmul(const nova_tensor_t *left, const nova_tensor_t *right);

#endif /* NOVA_ML_TAPE_H */
