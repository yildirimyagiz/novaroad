#include "nova_kernels.h"
#include "nova_tensor_ops.h"
#include <stdbool.h>
#include <stdlib.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR MATH - Phase 5 Standard
 * High-level orchestration for mathematical operations.
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaTensor *nova_op_matmul(NovaTensor *a, NovaTensor *b) {
  // 1. Result Allocation
  int64_t res_shape[2] = {a->shape[0], b->shape[1]};
  NovaTensor *res = nova_tensor_create(a->context, res_shape, 2, a->dtype);

  // 2. Kernel Execution (Backend Agnostic Dispatch)
  nova_kernel_matmul(a, b, res);

  // 3. Autograd record
  if (a->requires_grad || b->requires_grad) {
    res->requires_grad = true;
  }

  return res;
}

NovaTensor *nova_op_add(NovaTensor *a, NovaTensor *b) {
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  nova_kernel_add(a, b, res);

  if (a->requires_grad || b->requires_grad) {
    res->requires_grad = true;
  }

  return res;
}

NovaTensor *nova_op_mul(NovaTensor *a, NovaTensor *b) {
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  // nova_kernel_mul(a, b, res); // To be implemented
  return res;
}

NovaTensor *nova_op_relu(NovaTensor *x) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  // nova_kernel_relu(x, res); // To be implemented
  return res;
}
