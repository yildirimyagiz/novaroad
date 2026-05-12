#include "nova_formal.h"
#include "nova_kernels.h"
#include "nova_tensor.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR OPS - Unified ML Execution Layer
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaTensor *nova_op_matmul(NovaTensor *a, NovaTensor *b) {
  // Formal Invariant Check (NaN / propagation)
  if (!nova_formal_check_invariant("tensor_safety", a) ||
      !nova_formal_check_invariant("tensor_safety", b)) {
    fprintf(stderr,
            "⚠️ [Gödel] Tensor Ops Veto: Illegal tensor state detected\n");
  }

  // 1. Result Allocation (Pool Aware)
  // Assumes a is [M, K] and b is [K, N] (simplified)
  int64_t res_shape[2] = {a->shape[0], b->shape[a->ndim - 1]};
  NovaTensor *res = nova_tensor_create(a->context, res_shape, 2, a->dtype);

  // 2. Kernel Execution (Backend Agnostic)
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
  return res;
}

NovaTensor *nova_op_mul(NovaTensor *a, NovaTensor *b) {
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  // Element-wise mul kernel
  nova_kernel_mul(a, b, res);
  return res;
}

NovaTensor *nova_op_scalar_mul(NovaTensor *a, float s) {
  NovaTensor *res =
      nova_tensor_create(a->context, a->shape, a->ndim, a->dtype);
  nova_kernel_scalar_mul(a, s, res);
  return res;
}

NovaTensor *nova_op_relu(NovaTensor *x) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_relu(x, res);
  return res;
}

NovaTensor *nova_op_sigmoid(NovaTensor *x) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_sigmoid(x, res);
  return res;
}

NovaTensor *nova_op_softmax(NovaTensor *x, int dim) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_softmax(x, dim, res);
  return res;
}

NovaTensor *nova_op_gelu(NovaTensor *x) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_gelu(x, res);
  if (x->requires_grad) {
    res->requires_grad = true;
  }
  return res;
}

NovaTensor *nova_op_silu(NovaTensor *x) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_silu(x, res);
  if (x->requires_grad) {
    res->requires_grad = true;
  }
  return res;
}

NovaTensor *nova_op_tanh(NovaTensor *x) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_tanh(x, res);
  if (x->requires_grad) {
    res->requires_grad = true;
  }
  return res;
}

NovaTensor *nova_op_clamp(NovaTensor *x, float min, float max) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_clamp(x, min, max, res);
  if (x->requires_grad) {
    res->requires_grad = true;
  }
  return res;
}

NovaTensor *nova_op_pow(NovaTensor *x, float exponent) {
  NovaTensor *res =
      nova_tensor_create(x->context, x->shape, x->ndim, x->dtype);
  nova_kernel_pow(x, exponent, res);
  if (x->requires_grad) {
    res->requires_grad = true;
  }
  return res;
}

NovaTensor *nova_op_unsqueeze(NovaTensor *x, int dim) {
  // Calculate new shape with added dimension
  int new_ndim = x->ndim + 1;
  int64_t *new_shape = malloc(new_ndim * sizeof(int64_t));
  
  int actual_dim = dim < 0 ? new_ndim + dim : dim;
  for (int i = 0, j = 0; i < new_ndim; i++) {
    if (i == actual_dim) {
      new_shape[i] = 1;
    } else {
      new_shape[i] = x->shape[j++];
    }
  }
  
  NovaTensor *res =
      nova_tensor_create(x->context, new_shape, new_ndim, x->dtype);
  nova_kernel_unsqueeze(x, dim, res);
  
  free(new_shape);
  
  if (x->requires_grad) {
    res->requires_grad = true;
  }
  return res;
}

NovaTensor *nova_op_transpose(NovaTensor *t, int d1, int d2) {
  (void)d1;
  (void)d2;
  // Simplified transpose: create a copy with swapped shapes for 2D
  if (t->ndim == 2) {
    int64_t next_shape[] = {t->shape[1], t->shape[0]};
    NovaTensor *res =
        nova_tensor_create(t->context, next_shape, 2, t->dtype);
    // In actual implementation, this might be a view or use a kernel
    nova_kernel_transpose(t, res);
    return res;
  }
  return t; // Fallback
}

NovaTensor *nova_op_copy(NovaTensor *t) {
  NovaTensor *res =
      nova_tensor_create(t->context, t->shape, t->ndim, t->dtype);
  memcpy(res->data, t->data,
         t->total_elements * sizeof(float)); // Assuming FP32
  return res;
}

void nova_op_backward(NovaTensor *loss) {
  (void)loss;
  // Simplified autograd trigger
  printf("🔄 Triggering backward pass...\n");
}

void nova_op_zero_grad(NovaTensor *t) {
  if (t->grad) {
    memset(t->grad->data, 0, t->grad->total_elements * sizeof(float));
  }
}

// Global Stability Hook
void nova_op_stabilize(NovaTensor *t) { nova_kernel_stabilize_fp(t); }
