#ifndef NOVA_TENSOR_OPS_H
#define NOVA_TENSOR_OPS_H

#include "nova_tensor.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR OPS - High Level API for Models
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Math
NovaTensor *nova_op_matmul(NovaTensor *a, NovaTensor *b);
NovaTensor *nova_op_add(NovaTensor *a, NovaTensor *b);
NovaTensor *nova_op_mul(NovaTensor *a, NovaTensor *b);
NovaTensor *nova_op_scalar_mul(NovaTensor *a, float s);

// Activations
NovaTensor *nova_op_relu(NovaTensor *x);
NovaTensor *nova_op_sigmoid(NovaTensor *x);
NovaTensor *nova_op_softmax(NovaTensor *x, int dim);

// Utils
NovaTensor *nova_op_transpose(NovaTensor *t, int d1, int d2);
NovaTensor *nova_op_copy(NovaTensor *t);

// Autograd
void nova_op_backward(NovaTensor *loss);
void nova_op_zero_grad(NovaTensor *t);

// Factory helpers (declared in nova_tensor.h)

#endif // NOVA_TENSOR_OPS_H
