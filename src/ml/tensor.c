// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA TENSOR BACKEND  v2.0                                             ║
// ║  C implementation of multi-dimensional arrays with autograd             ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#include "ml/tensor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// TENSOR DATA STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════════════════════════════
// TENSOR LIFECYCLE
// ══════════════════════════════════════════════════════════════════════════════

nova_tensor_t *nova_tensor_create(nova_dtype_t dtype, size_t ndims, const size_t *shape)
{
    nova_tensor_t *tensor = calloc(1, sizeof(nova_tensor_t));
    if (!tensor)
        return NULL;

    tensor->dtype = dtype;
    tensor->ndims = ndims;
    tensor->shape = malloc(sizeof(size_t) * ndims);
    tensor->strides = malloc(sizeof(size_t) * ndims);

    if (!tensor->shape || !tensor->strides) {
        nova_tensor_destroy(tensor);
        return NULL;
    }

    memcpy(tensor->shape, shape, sizeof(size_t) * ndims);

    // Compute strides (row-major)
    tensor->strides[ndims - 1] = 1;
    for (int i = ndims - 2; i >= 0; i--) {
        tensor->strides[i] = tensor->strides[i + 1] * shape[i + 1];
    }

    // Compute total size
    size_t total_elements = 1;
    for (size_t i = 0; i < ndims; i++) {
        total_elements *= shape[i];
    }

    tensor->data_size = total_elements * nova_dtype_size(dtype);
    tensor->data = calloc(1, tensor->data_size);

    if (!tensor->data) {
        nova_tensor_destroy(tensor);
        return NULL;
    }

    tensor->requires_grad = false;
    tensor->grad_fn = NULL;
    tensor->grad = NULL;
    tensor->name = NULL;

    return tensor;
}

nova_tensor_t *nova_tensor_zeros(nova_dtype_t dtype, size_t ndims, const size_t *shape)
{
    nova_tensor_t *tensor = nova_tensor_create(dtype, ndims, shape);
    if (tensor) {
        memset(tensor->data, 0, tensor->data_size);
    }
    return tensor;
}

nova_tensor_t *nova_tensor_ones(nova_dtype_t dtype, size_t ndims, const size_t *shape)
{
    nova_tensor_t *tensor = nova_tensor_create(dtype, ndims, shape);
    if (!tensor)
        return NULL;

    size_t element_size = nova_dtype_size(dtype);
    size_t num_elements = tensor->data_size / element_size;

    for (size_t i = 0; i < num_elements; i++) {
        void *element_ptr = (char *) tensor->data + i * element_size;
        switch (dtype) {
        case NOVA_DTYPE_F32:
            *(float *) element_ptr = 1.0f;
            break;
        case NOVA_DTYPE_F64:
            *(double *) element_ptr = 1.0;
            break;
        case NOVA_DTYPE_I32:
            *(int32_t *) element_ptr = 1;
            break;
        case NOVA_DTYPE_I64:
            *(int64_t *) element_ptr = 1;
            break;
        }
    }

    return tensor;
}

void nova_tensor_destroy(nova_tensor_t *tensor)
{
    if (!tensor)
        return;

    free(tensor->shape);
    free(tensor->strides);
    free(tensor->data);

    if (tensor->grad_fn) {
        nova_grad_fn_destroy(tensor->grad_fn);
    }

    if (tensor->grad) {
        nova_tensor_destroy(tensor->grad);
    }

    free(tensor->name);
    free(tensor);
}

// ══════════════════════════════════════════════════════════════════════════════
// TENSOR OPERATIONS
// ══════════════════════════════════════════════════════════════════════════════

nova_tensor_t *nova_tensor_add(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!nova_tensor_shapes_equal(a, b)) {
        return NULL;
    }

    nova_tensor_t *result = nova_tensor_create(a->dtype, a->ndims, a->shape);
    if (!result)
        return NULL;

    result->requires_grad = a->requires_grad || b->requires_grad;

    size_t element_size = nova_dtype_size(a->dtype);
    size_t num_elements = a->data_size / element_size;

    for (size_t i = 0; i < num_elements; i++) {
        void *a_ptr = (char *) a->data + i * element_size;
        void *b_ptr = (char *) b->data + i * element_size;
        void *r_ptr = (char *) result->data + i * element_size;

        switch (a->dtype) {
        case NOVA_DTYPE_F32:
            *(float *) r_ptr = *(float *) a_ptr + *(float *) b_ptr;
            break;
        case NOVA_DTYPE_F64:
            *(double *) r_ptr = *(double *) a_ptr + *(double *) b_ptr;
            break;
        case NOVA_DTYPE_I32:
            *(int32_t *) r_ptr = *(int32_t *) a_ptr + *(int32_t *) b_ptr;
            break;
        case NOVA_DTYPE_I64:
            *(int64_t *) r_ptr = *(int64_t *) a_ptr + *(int64_t *) b_ptr;
            break;
        }
    }

    // Set up gradient function
    if (result->requires_grad) {
        result->grad_fn = nova_grad_fn_add(a, b);
    }

    return result;
}

nova_tensor_t *nova_tensor_matmul(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (a->ndims != 2 || b->ndims != 2 || a->shape[1] != b->shape[0]) {
        return NULL;
    }

    size_t result_shape[2] = {a->shape[0], b->shape[1]};
    nova_tensor_t *result = nova_tensor_create(a->dtype, 2, result_shape);
    if (!result)
        return NULL;

    result->requires_grad = a->requires_grad || b->requires_grad;

    // Simple matrix multiplication (can be optimized with BLAS)
    for (size_t i = 0; i < a->shape[0]; i++) {
        for (size_t j = 0; j < b->shape[1]; j++) {
            float sum = 0.0f;
            for (size_t k = 0; k < a->shape[1]; k++) {
                float a_val = nova_tensor_get_f32(a, (size_t[]) {i, k});
                float b_val = nova_tensor_get_f32(b, (size_t[]) {k, j});
                sum += a_val * b_val;
            }
            nova_tensor_set_f32(result, (size_t[]) {i, j}, sum);
        }
    }

    // Set up gradient function
    if (result->requires_grad) {
        result->grad_fn = nova_grad_fn_matmul(a, b);
    }

    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
// AUTOGRAD SYSTEM
// ══════════════════════════════════════════════════════════════════════════════

nova_grad_fn_t *nova_grad_fn_create(nova_grad_fn_type_t type, nova_tensor_t *input1,
                                    nova_tensor_t *input2)
{
    nova_grad_fn_t *fn = calloc(1, sizeof(nova_grad_fn_t));
    if (!fn)
        return NULL;

    fn->type = type;
    fn->input1 = input1 ? nova_tensor_clone(input1) : NULL;
    fn->input2 = input2 ? nova_tensor_clone(input2) : NULL;

    switch (type) {
    case NOVA_GRAD_FN_ADD:
        fn->backward = nova_grad_fn_add_backward;
        break;
    case NOVA_GRAD_FN_MATMUL:
        fn->backward = nova_grad_fn_matmul_backward;
        break;
    default:
        break;
    }

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

nova_grad_fn_t *nova_grad_fn_add(const nova_tensor_t *a, const nova_tensor_t *b)
{
    return nova_grad_fn_create(NOVA_GRAD_FN_ADD, a, b);
}

nova_grad_fn_t *nova_grad_fn_matmul(const nova_tensor_t *a, const nova_tensor_t *b)
{
    return nova_grad_fn_create(NOVA_GRAD_FN_MATMUL, a, b);
}

void nova_grad_fn_add_backward(nova_grad_fn_t *fn, nova_tensor_t *grad_output,
                               nova_tensor_t **grad_input1, nova_tensor_t **grad_input2)
{
    // d(a+b)/da = 1, d(a+b)/db = 1
    *grad_input1 = nova_tensor_clone(grad_output);
    *grad_input2 = nova_tensor_clone(grad_output);
}

void nova_grad_fn_matmul_backward(nova_grad_fn_t *fn, nova_tensor_t *grad_output,
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

// ══════════════════════════════════════════════════════════════════════════════
// BACKWARD PASS (AUTOGRAD)
// ══════════════════════════════════════════════════════════════════════════════

int nova_tensor_backward(nova_tensor_t *tensor)
{
    if (!tensor->requires_grad) {
        return -1; // No gradient computation needed
    }

    // Create gradient output (all ones, same shape as tensor)
    nova_tensor_t *grad_output = nova_tensor_ones(tensor->dtype, tensor->ndims, tensor->shape);
    if (!grad_output)
        return -1;

    int result = nova_tensor_backward_with_grad(tensor, grad_output);
    nova_tensor_destroy(grad_output);

    return result;
}

int nova_tensor_backward_with_grad(nova_tensor_t *tensor, nova_tensor_t *grad_output)
{
    if (!tensor->requires_grad || !tensor->grad_fn) {
        return 0;
    }

    // Compute gradients for inputs
    nova_tensor_t *grad_input1 = NULL;
    nova_tensor_t *grad_input2 = NULL;

    tensor->grad_fn->backward(tensor->grad_fn, grad_output, &grad_input1, &grad_input2);

    // Store gradient
    tensor->grad = nova_tensor_clone(grad_output);

    // Propagate to inputs
    if (tensor->grad_fn->input1 && grad_input1) {
        nova_tensor_backward_with_grad(tensor->grad_fn->input1, grad_input1);
        nova_tensor_destroy(grad_input1);
    }

    if (tensor->grad_fn->input2 && grad_input2) {
        nova_tensor_backward_with_grad(tensor->grad_fn->input2, grad_input2);
        nova_tensor_destroy(grad_input2);
    }

    return 0;
}

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

bool nova_tensor_shapes_equal(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (a->ndims != b->ndims)
        return false;

    for (size_t i = 0; i < a->ndims; i++) {
        if (a->shape[i] != b->shape[i])
            return false;
    }

    return true;
}

nova_tensor_t *nova_tensor_clone(const nova_tensor_t *tensor)
{
    nova_tensor_t *clone = nova_tensor_create(tensor->dtype, tensor->ndims, tensor->shape);
    if (!clone)
        return NULL;

    memcpy(clone->data, tensor->data, tensor->data_size);
    clone->requires_grad = tensor->requires_grad;
    clone->grad_fn = tensor->grad_fn ? nova_grad_fn_clone(tensor->grad_fn) : NULL;

    if (tensor->grad) {
        clone->grad = nova_tensor_clone(tensor->grad);
    }

    if (tensor->name) {
        clone->name = strdup(tensor->name);
    }

    return clone;
}

nova_tensor_t *nova_tensor_transpose(const nova_tensor_t *tensor)
{
    if (tensor->ndims != 2)
        return NULL;

    size_t new_shape[2] = {tensor->shape[1], tensor->shape[0]};
    nova_tensor_t *result = nova_tensor_create(tensor->dtype, 2, new_shape);
    if (!result)
        return NULL;

    // Simple transpose (can be optimized)
    for (size_t i = 0; i < tensor->shape[0]; i++) {
        for (size_t j = 0; j < tensor->shape[1]; j++) {
            float val = nova_tensor_get_f32(tensor, (size_t[]) {i, j});
            nova_tensor_set_f32(result, (size_t[]) {j, i}, val);
        }
    }

    return result;
}

// Type-specific accessors (simplified - only f32 for now)
float nova_tensor_get_f32(const nova_tensor_t *tensor, const size_t *indices)
{
    size_t offset = 0;
    for (size_t i = 0; i < tensor->ndims; i++) {
        offset += indices[i] * tensor->strides[i];
    }
    return ((float *) tensor->data)[offset];
}

void nova_tensor_set_f32(nova_tensor_t *tensor, const size_t *indices, float value)
{
    size_t offset = 0;
    for (size_t i = 0; i < tensor->ndims; i++) {
        offset += indices[i] * tensor->strides[i];
    }
    ((float *) tensor->data)[offset] = value;
}

size_t nova_dtype_size(nova_dtype_t dtype)
{
    switch (dtype) {
    case NOVA_DTYPE_F32:
        return 4;
    case NOVA_DTYPE_F64:
        return 8;
    case NOVA_DTYPE_I32:
        return 4;
    case NOVA_DTYPE_I64:
        return 8;
    default:
        return 0;
    }
}

// Placeholder implementations
nova_grad_fn_t *nova_grad_fn_clone(nova_grad_fn_t *fn)
{
    return nova_grad_fn_create(fn->type, fn->input1, fn->input2);
}
