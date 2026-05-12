#include "nova_allocator.h"
#include "ml/nova_tensor.h"
#include "nova_tensor_ops.h"
#include <stdlib.h>
#include <string.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR LAYOUT - Phase 5 Standard
 * Shape manipulation, striding, and views.
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* nova_op_transpose is defined in nova_tensor_ops.c — not duplicated here */

NovaTensor *nova_op_view(NovaTensor *t, int64_t *new_shape,
                             int new_ndim) {
  // Check if total elements match
  size_t new_total = 1;
  for (int i = 0; i < new_ndim; i++)
    new_total *= new_shape[i];

  if (new_total != t->total_elements)
    return NULL;

  NovaTensor *view =
      (NovaTensor *)nova_malloc(sizeof(NovaTensor), MEM_TAG_GENERAL);
  view->context = t->context;
  view->device = t->device;
  view->dtype = t->dtype;
  view->ndim = new_ndim;
  view->shape =
      (int64_t *)nova_malloc(new_ndim * sizeof(int64_t), MEM_TAG_GENERAL);
  view->strides =
      (int64_t *)nova_malloc(new_ndim * sizeof(int64_t), MEM_TAG_GENERAL);

  memcpy(view->shape, new_shape, new_ndim * sizeof(int64_t));

  // Calculate new strides...
  int64_t stride = 1;
  for (int i = new_ndim - 1; i >= 0; i--) {
    view->strides[i] = stride;
    stride *= new_shape[i];
  }

  view->total_elements = t->total_elements;
  view->data = t->data; // Shared pointer
  view->is_view = true;
  view->requires_grad = false; // Views usually don't have their own grads

  return view;
}
