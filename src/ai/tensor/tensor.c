/**
 * @file tensor.c
 * @brief Tensor implementation
 */

#include "ai/tensor.h"
#include "ai/autograd.h"
#include "../autograd/autograd.h"
#include "../autograd/tape.h"
#include "std/alloc.h"
#include "std/collections/vec.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Global gradient tape for automatic recording
static nova_grad_tape_t *g_current_tape = NULL;

void nova_tensor_set_global_tape(nova_grad_tape_t *tape)
{
    g_current_tape = tape;
}


nova_tensor_t *nova_tensor_create(const size_t *shape, size_t ndim, nova_dtype_t dtype)
{
    nova_tensor_t *tensor = nova_alloc(sizeof(nova_tensor_t));
    if (!tensor) return NULL;
    
    tensor->dtype = dtype;
    tensor->device = NOVA_DEVICE_CPU;  // Default to CPU
    tensor->ndim = ndim;
    tensor->shape = nova_alloc(sizeof(size_t) * ndim);
    
    size_t total_size = 1;
    for (size_t i = 0; i < ndim; i++) {
        tensor->shape[i] = shape[i];
        total_size *= shape[i];
    }
    
    tensor->size = total_size;
    tensor->data = nova_alloc(total_size * sizeof(float)); // Simplified
    
    // Initialize autograd fields
    tensor->requires_grad = false;
    tensor->grad = NULL;
    tensor->grad_fn = NULL;
    tensor->grad_accum_count = 0;
    
    return tensor;
}

nova_tensor_t *nova_tensor_zeros(const size_t *shape, size_t ndim, nova_dtype_t dtype)
{
    nova_tensor_t *tensor = nova_tensor_create(shape, ndim, dtype);
    if (tensor) {
        memset(tensor->data, 0, tensor->size * sizeof(float));
    }
    return tensor;
}

nova_tensor_t *nova_tensor_ones(const size_t *shape, size_t ndim, nova_dtype_t dtype)
{
    nova_tensor_t *tensor = nova_tensor_create(shape, ndim, dtype);
    if (tensor) {
        for (size_t i = 0; i < tensor->size; i++) {
            ((float*)tensor->data)[i] = 1.0f;
        }
    }
    return tensor;
}

nova_tensor_t *nova_tensor_randn(const size_t *shape, size_t ndim, nova_dtype_t dtype)
{
    nova_tensor_t *tensor = nova_tensor_create(shape, ndim, dtype);
    if (tensor) {
        /* Box-Muller transform for normal distribution */
        float *data = (float *)tensor->data;
        for (size_t i = 0; i + 1 < tensor->size; i += 2) {
            float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
            float u2 = (float)rand() / (float)RAND_MAX;
            float r  = sqrtf(-2.0f * logf(u1));
            float th = 6.28318530718f * u2;
            data[i]     = r * cosf(th);
            data[i + 1] = r * sinf(th);
        }
        if (tensor->size % 2 != 0) {
            float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
            float u2 = (float)rand() / (float)RAND_MAX;
            data[tensor->size - 1] = sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
        }
        (void)dtype; /* already set in create */
    }
    return tensor;
}

nova_tensor_t *nova_tensor_from_data(const void *data, const size_t *shape, 
                                      size_t ndim, nova_dtype_t dtype)
{
    nova_tensor_t *tensor = nova_tensor_create(shape, ndim, dtype);
    if (tensor && data) {
        memcpy(tensor->data, data, tensor->size * sizeof(float));
    }
    return tensor;
}

nova_tensor_t *nova_tensor_add(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!a || !b) return NULL;
    
    // For now, just do element-wise addition
    // In a full implementation, this would handle broadcasting
    if (a->size != b->size) return NULL;
    
    size_t shape[1] = {a->size};
    nova_tensor_t *result = nova_tensor_create(shape, 1, a->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < a->size; i++) {
        ((float*)result->data)[i] = ((float*)a->data)[i] + ((float*)b->data)[i];
    }
    
    // Record operation for autograd if needed
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape)) {
        if (nova_tensor_requires_grad(a) || nova_tensor_requires_grad(b)) {
            nova_vec_t *inputs = nova_vec_new();
            nova_vec_push(inputs, (void*)a);
            nova_vec_push(inputs, (void*)b);
            
            nova_vec_t *outputs = nova_vec_new();
            nova_vec_push(outputs, result);
            
            nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_ADD, inputs, outputs);
            if (grad_fn) {
                result->grad_fn = grad_fn;
                result->requires_grad = true;
                nova_grad_tape_record_operation(g_current_tape, grad_fn, result);
            }
            
            nova_vec_destroy(inputs);
            nova_vec_destroy(outputs);
        }
    }
    
    return result;
}

nova_tensor_t *nova_tensor_mul(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!a || !b) return NULL;
    
    if (a->size != b->size) return NULL;
    
    size_t shape[1] = {a->size};
    nova_tensor_t *result = nova_tensor_create(shape, 1, a->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < a->size; i++) {
        ((float*)result->data)[i] = ((float*)a->data)[i] * ((float*)b->data)[i];
    }
    
    // Record operation for autograd if needed
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape)) {
        if (nova_tensor_requires_grad(a) || nova_tensor_requires_grad(b)) {
            nova_vec_t *inputs = nova_vec_new();
            nova_vec_push(inputs, (void*)a);
            nova_vec_push(inputs, (void*)b);
            
            nova_vec_t *outputs = nova_vec_new();
            nova_vec_push(outputs, result);
            
            nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_MUL, inputs, outputs);
            if (grad_fn) {
                result->grad_fn = grad_fn;
                result->requires_grad = true;
                nova_grad_tape_record_operation(g_current_tape, grad_fn, result);
            }
            
            nova_vec_destroy(inputs);
            nova_vec_destroy(outputs);
        }
    }
    
    return result;
}

nova_tensor_t *nova_tensor_clone(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *clone = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!clone) return NULL;
    
    // Copy data
    memcpy(clone->data, tensor->data, tensor->size * sizeof(float));
    
    // Copy autograd fields
    clone->requires_grad = tensor->requires_grad;
    if (tensor->grad) {
        clone->grad = nova_tensor_clone(tensor->grad);
    }
    
    return clone;
}

// ============================================================================
// Tensor Properties
// ============================================================================

size_t nova_tensor_ndim(const nova_tensor_t *tensor)
{
    return tensor ? tensor->ndim : 0;
}

const size_t *nova_tensor_shape(const nova_tensor_t *tensor)
{
    return tensor ? tensor->shape : NULL;
}

size_t nova_tensor_size(const nova_tensor_t *tensor, size_t dim)
{
    if (!tensor || dim >= tensor->ndim) return 0;
    return tensor->shape[dim];
}

size_t nova_tensor_numel(const nova_tensor_t *tensor)
{
    return tensor ? tensor->size : 0;
}

nova_dtype_t nova_tensor_dtype(const nova_tensor_t *tensor)
{
    return tensor ? tensor->dtype : NOVA_DTYPE_FLOAT32;
}

nova_device_t nova_tensor_device(const nova_tensor_t *tensor)
{
    return tensor ? tensor->device : NOVA_DEVICE_CPU;
}

void *nova_tensor_data(nova_tensor_t *tensor)
{
    return tensor ? tensor->data : NULL;
}

size_t nova_tensor_element_size(const nova_tensor_t *tensor)
{
    if (!tensor) return 0;
    
    switch (tensor->dtype) {
        case NOVA_DTYPE_FLOAT32: return 4;
        case NOVA_DTYPE_FLOAT64: return 8;
        case NOVA_DTYPE_INT32: return 4;
        case NOVA_DTYPE_INT64: return 8;
        case NOVA_DTYPE_INT8: return 1;
        case NOVA_DTYPE_UINT8: return 1;
        case NOVA_DTYPE_BOOL: return 1;
        default: return 4; // Default to float32 size
    }
}

// ============================================================================
// Element Access
// ============================================================================

float nova_tensor_get_f32(const nova_tensor_t *tensor, const size_t *indices)
{
    if (!tensor || !indices) return 0.0f;
    
    size_t flat_index = 0;
    size_t stride = 1;
    
    // Convert multi-dimensional indices to flat index
    for (int i = (int)tensor->ndim - 1; i >= 0; i--) {
        flat_index += indices[i] * stride;
        stride *= tensor->shape[i];
    }
    
    if (flat_index >= tensor->size) return 0.0f;
    
    return ((float*)tensor->data)[flat_index];
}

void nova_tensor_set_f32(nova_tensor_t *tensor, const size_t *indices, float value)
{
    if (!tensor || !indices) return;
    
    size_t flat_index = 0;
    size_t stride = 1;
    
    // Convert multi-dimensional indices to flat index
    for (int i = (int)tensor->ndim - 1; i >= 0; i--) {
        flat_index += indices[i] * stride;
        stride *= tensor->shape[i];
    }
    
    if (flat_index >= tensor->size) return;
    
    ((float*)tensor->data)[flat_index] = value;
}

float nova_tensor_get_item(const nova_tensor_t *tensor, size_t index)
{
    if (!tensor || index >= tensor->size) return 0.0f;
    return ((float*)tensor->data)[index];
}

void nova_tensor_set_item(nova_tensor_t *tensor, size_t index, float value)
{
    if (!tensor || index >= tensor->size) return;
    ((float*)tensor->data)[index] = value;
}

// ============================================================================
// Tensor Reshaping & Manipulation
// ============================================================================

nova_tensor_t *nova_tensor_reshape(nova_tensor_t *tensor, const size_t *new_shape, size_t new_ndim)
{
    if (!tensor || !new_shape) return NULL;
    
    // Calculate total elements
    size_t total_elements = 1;
    for (size_t i = 0; i < new_ndim; i++) {
        total_elements *= new_shape[i];
    }
    
    if (total_elements != tensor->size) return NULL;
    
    // Create new shape array
    size_t *reshaped_shape = nova_alloc(new_ndim * sizeof(size_t));
    if (!reshaped_shape) return NULL;
    
    memcpy(reshaped_shape, new_shape, new_ndim * sizeof(size_t));
    
    // Update tensor
    nova_free(tensor->shape);
    tensor->shape = reshaped_shape;
    tensor->ndim = new_ndim;
    
    return tensor;
}

nova_tensor_t *nova_tensor_transpose(const nova_tensor_t *tensor, size_t dim0, size_t dim1)
{
    if (!tensor || dim0 >= tensor->ndim || dim1 >= tensor->ndim) return NULL;
    
    // Create new shape with swapped dimensions
    size_t *new_shape = nova_alloc(tensor->ndim * sizeof(size_t));
    if (!new_shape) return NULL;
    
    memcpy(new_shape, tensor->shape, tensor->ndim * sizeof(size_t));
    size_t temp = new_shape[dim0];
    new_shape[dim0] = new_shape[dim1];
    new_shape[dim1] = temp;
    
    nova_tensor_t *result = nova_tensor_create(new_shape, tensor->ndim, tensor->dtype);
    nova_free(new_shape);
    
    if (!result) return NULL;
    
    // For now, only handle 2D tensors
    if (tensor->ndim == 2) {
        for (size_t i = 0; i < tensor->shape[0]; i++) {
            for (size_t j = 0; j < tensor->shape[1]; j++) {
                size_t src_idx = i * tensor->shape[1] + j;
                size_t dst_idx = j * tensor->shape[0] + i;
                ((float*)result->data)[dst_idx] = ((float*)tensor->data)[src_idx];
            }
        }
    } else {
        // TODO: Implement general transpose
        nova_tensor_destroy(result);
        return NULL;
    }
    
    return result;
}

nova_tensor_t *nova_tensor_permute(const nova_tensor_t *tensor, const size_t *dims)
{
    // TODO: Implement general permutation
    return NULL;
}

nova_tensor_t *nova_tensor_squeeze(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    // Count non-unit dimensions
    size_t new_ndim = 0;
    for (size_t i = 0; i < tensor->ndim; i++) {
        if (tensor->shape[i] != 1) new_ndim++;
    }
    
    if (new_ndim == tensor->ndim) {
        // No dimensions to squeeze
        return nova_tensor_clone(tensor);
    }
    
    size_t *new_shape = nova_alloc(new_ndim * sizeof(size_t));
    if (!new_shape) return NULL;
    
    size_t idx = 0;
    for (size_t i = 0; i < tensor->ndim; i++) {
        if (tensor->shape[i] != 1) {
            new_shape[idx++] = tensor->shape[i];
        }
    }
    
    nova_tensor_t *result = nova_tensor_create(new_shape, new_ndim, tensor->dtype);
    nova_free(new_shape);
    
    if (!result) return NULL;
    
    memcpy(result->data, tensor->data, tensor->size * sizeof(float));
    return result;
}

nova_tensor_t *nova_tensor_unsqueeze(const nova_tensor_t *tensor, size_t dim)
{
    if (!tensor || dim > tensor->ndim) return NULL;
    
    size_t new_ndim = tensor->ndim + 1;
    size_t *new_shape = nova_alloc(new_ndim * sizeof(size_t));
    if (!new_shape) return NULL;
    
    // Copy shape with new dimension inserted
    for (size_t i = 0; i < dim; i++) {
        new_shape[i] = tensor->shape[i];
    }
    new_shape[dim] = 1;
    for (size_t i = dim + 1; i < new_ndim; i++) {
        new_shape[i] = tensor->shape[i - 1];
    }
    
    nova_tensor_t *result = nova_tensor_create(new_shape, new_ndim, tensor->dtype);
    nova_free(new_shape);
    
    if (!result) return NULL;
    
    memcpy(result->data, tensor->data, tensor->size * sizeof(float));
    return result;
}

nova_tensor_t *nova_tensor_flatten(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    size_t new_shape[1] = {tensor->size};
    nova_tensor_t *result = nova_tensor_create(new_shape, 1, tensor->dtype);
    
    if (!result) return NULL;
    
    memcpy(result->data, tensor->data, tensor->size * sizeof(float));
    return result;
}

nova_tensor_t *nova_tensor_slice(const nova_tensor_t *tensor, size_t dim, 
                                size_t start, size_t end)
{
    // TODO: Implement tensor slicing
    return NULL;
}

void nova_tensor_destroy(nova_tensor_t *tensor)
{
    if (tensor) {
        nova_free(tensor->shape);
        nova_free(tensor->data);
        
        // Clean up autograd fields
        if (tensor->grad) {
            nova_tensor_destroy(tensor->grad);
        }
        
        // Clean up gradient function
        if (tensor->grad_fn) {
            nova_grad_fn_destroy(tensor->grad_fn);
            tensor->grad_fn = NULL;
        }
        
        nova_free(tensor);
    }
}

// ============================================================================
// Additional tensor operations
// ============================================================================

nova_tensor_t *nova_tensor_matmul(const nova_tensor_t *a, const nova_tensor_t *b)
{
    // Simplified 2D matrix multiplication
    if (!a || !b || a->ndim != 2 || b->ndim != 2 || a->shape[1] != b->shape[0]) {
        return NULL;
    }
    
    size_t m = a->shape[0], k = a->shape[1], n = b->shape[1];
    size_t result_shape[2] = {m, n};
    nova_tensor_t *result = nova_tensor_create(result_shape, 2, a->dtype);
    
    if (!result) return NULL;
    
    // Simple matrix multiplication
    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n; j++) {
            float sum = 0.0f;
            for (size_t l = 0; l < k; l++) {
                sum += ((float*)a->data)[i * k + l] * ((float*)b->data)[l * n + j];
            }
            ((float*)result->data)[i * n + j] = sum;
        }
    }
    
    // Record operation for autograd if needed
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape)) {
        if (nova_tensor_requires_grad(a) || nova_tensor_requires_grad(b)) {
            // TODO: Implement matmul backward
            result->requires_grad = true;
        }
    }
    
    return result;
}

float nova_tensor_sum(const nova_tensor_t *tensor)
{
    if (!tensor) return 0.0f;
    
    float sum = 0.0f;
    for (size_t i = 0; i < tensor->size; i++) {
        sum += ((float*)tensor->data)[i];
    }
    return sum;
}

float nova_tensor_mean(const nova_tensor_t *tensor)
{
    if (!tensor || tensor->size == 0) return 0.0f;
    return nova_tensor_sum(tensor) / (float)tensor->size;
}

float nova_tensor_max(const nova_tensor_t *tensor)
{
    if (!tensor || tensor->size == 0) return 0.0f;
    
    float max_val = ((float*)tensor->data)[0];
    for (size_t i = 1; i < tensor->size; i++) {
        float val = ((float*)tensor->data)[i];
        if (val > max_val) max_val = val;
    }
    return max_val;
}

float nova_tensor_min(const nova_tensor_t *tensor)
{
    if (!tensor || tensor->size == 0) return 0.0f;
    
    float min_val = ((float*)tensor->data)[0];
    for (size_t i = 1; i < tensor->size; i++) {
        float val = ((float*)tensor->data)[i];
        if (val < min_val) min_val = val;
    }
    return min_val;
}

size_t nova_tensor_argmax(const nova_tensor_t *tensor)
{
    if (!tensor || tensor->size == 0) return 0;
    
    size_t max_idx = 0;
    float max_val = ((float*)tensor->data)[0];
    for (size_t i = 1; i < tensor->size; i++) {
        float val = ((float*)tensor->data)[i];
        if (val > max_val) {
            max_val = val;
            max_idx = i;
        }
    }
    return max_idx;
}

size_t nova_tensor_argmin(const nova_tensor_t *tensor)
{
    if (!tensor || tensor->size == 0) return 0;
    
    size_t min_idx = 0;
    float min_val = ((float*)tensor->data)[0];
    for (size_t i = 1; i < tensor->size; i++) {
        float val = ((float*)tensor->data)[i];
        if (val < min_val) {
            min_val = val;
            min_idx = i;
        }
    }
    return min_idx;
}

void nova_tensor_print(const nova_tensor_t *tensor)
{
    if (!tensor) {
        printf("NULL tensor\n");
        return;
    }
    
    printf("Tensor(shape=[");
    for (size_t i = 0; i < tensor->ndim; i++) {
        printf("%zu", tensor->shape[i]);
        if (i < tensor->ndim - 1) printf(",");
    }
    printf("], dtype=%d, device=%d, requires_grad=%s)\n",
           tensor->dtype, tensor->device, tensor->requires_grad ? "true" : "false");
    
    // Print first few elements
    printf("Data: [");
    size_t max_print = tensor->size > 10 ? 10 : tensor->size;
    for (size_t i = 0; i < max_print; i++) {
        printf("%.3f", ((float*)tensor->data)[i]);
        if (i < max_print - 1) printf(", ");
    }
    if (tensor->size > 10) printf(" ...");
    printf("]\n");
}

bool nova_tensor_same_shape(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!a || !b || a->ndim != b->ndim) return false;
    
    for (size_t i = 0; i < a->ndim; i++) {
        if (a->shape[i] != b->shape[i]) return false;
    }
    return true;
}

const char *nova_dtype_to_string(nova_dtype_t dtype)
{
    switch (dtype) {
        case NOVA_DTYPE_FLOAT32: return "float32";
        case NOVA_DTYPE_FLOAT64: return "float64";
        case NOVA_DTYPE_INT32: return "int32";
        case NOVA_DTYPE_INT64: return "int64";
        case NOVA_DTYPE_UINT8: return "uint8";
        case NOVA_DTYPE_INT8: return "int8";
        case NOVA_DTYPE_BOOL: return "bool";
        default: return "unknown";
    }
}

// ============================================================================
// Stub implementations for remaining functions
// ============================================================================

nova_tensor_t *nova_tensor_sub(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!a || !b || !nova_tensor_same_shape(a, b)) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(a->shape, a->ndim, a->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < a->size; i++) {
        ((float*)result->data)[i] = ((float*)a->data)[i] - ((float*)b->data)[i];
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape)) {
        if (nova_tensor_requires_grad(a) || nova_tensor_requires_grad(b)) {
            nova_vec_t *inputs = nova_vec_new();
            nova_vec_t *outputs = nova_vec_new();
            
            if (nova_tensor_requires_grad(a)) nova_vec_push(inputs, (void*)a);
            if (nova_tensor_requires_grad(b)) nova_vec_push(inputs, (void*)b);
            nova_vec_push(outputs, (void*)result);
            
            nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_SUB, inputs, outputs);
            result->grad_fn = grad_fn;
            result->requires_grad = true;
            
            nova_vec_destroy(inputs);
            nova_vec_destroy(outputs);
        }
    }
    
    return result;
}

nova_tensor_t *nova_tensor_div(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!a || !b || !nova_tensor_same_shape(a, b)) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(a->shape, a->ndim, a->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < a->size; i++) {
        float b_val = ((float*)b->data)[i];
        if (b_val == 0.0f) {
            nova_tensor_destroy(result);
            return NULL; // Division by zero
        }
        ((float*)result->data)[i] = ((float*)a->data)[i] / b_val;
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape)) {
        if (nova_tensor_requires_grad(a) || nova_tensor_requires_grad(b)) {
            nova_vec_t *inputs = nova_vec_new();
            nova_vec_t *outputs = nova_vec_new();
            
            if (nova_tensor_requires_grad(a)) nova_vec_push(inputs, (void*)a);
            if (nova_tensor_requires_grad(b)) nova_vec_push(inputs, (void*)b);
            nova_vec_push(outputs, (void*)result);
            
            nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_DIV, inputs, outputs);
            result->grad_fn = grad_fn;
            result->requires_grad = true;
            
            nova_vec_destroy(inputs);
            nova_vec_destroy(outputs);
        }
    }
    
    return result;
}

nova_tensor_t *nova_tensor_add_scalar(const nova_tensor_t *tensor, float scalar)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = ((float*)tensor->data)[i] + scalar;
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_ADD_SCALAR, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_mul_scalar(const nova_tensor_t *tensor, float scalar)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = ((float*)tensor->data)[i] * scalar;
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_MUL_SCALAR, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

float nova_tensor_dot(const nova_tensor_t *a, const nova_tensor_t *b)
{
    if (!a || !b || a->size != b->size) return 0.0f;
    
    float result = 0.0f;
    for (size_t i = 0; i < a->size; i++) {
        result += ((float*)a->data)[i] * ((float*)b->data)[i];
    }
    
    return result;
}

nova_tensor_t *nova_tensor_exp(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = expf(((float*)tensor->data)[i]);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_EXP, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_log(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        float val = ((float*)tensor->data)[i];
        if (val <= 0.0f) {
            nova_tensor_destroy(result);
            return NULL; // Log of non-positive number
        }
        ((float*)result->data)[i] = logf(val);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_LOG, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_sqrt(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        float val = ((float*)tensor->data)[i];
        if (val < 0.0f) {
            nova_tensor_destroy(result);
            return NULL; // Square root of negative number
        }
        ((float*)result->data)[i] = sqrtf(val);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_SQRT, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_pow(const nova_tensor_t *tensor, float exponent)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = powf(((float*)tensor->data)[i], exponent);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_POW, inputs, outputs);
        // Store exponent in op_data
        grad_fn->op_data = nova_alloc(sizeof(float));
        if (grad_fn->op_data) {
            *(float*)grad_fn->op_data = exponent;
        }
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_abs(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = fabsf(((float*)tensor->data)[i]);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_ABS, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_sin(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = sinf(((float*)tensor->data)[i]);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_SIN, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_cos(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = cosf(((float*)tensor->data)[i]);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_COS, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_tanh(const nova_tensor_t *tensor)
{
    if (!tensor) return NULL;
    
    nova_tensor_t *result = nova_tensor_create(tensor->shape, tensor->ndim, tensor->dtype);
    if (!result) return NULL;
    
    for (size_t i = 0; i < tensor->size; i++) {
        ((float*)result->data)[i] = tanhf(((float*)tensor->data)[i]);
    }
    
    // Record operation for autograd
    if (g_current_tape && nova_grad_tape_is_recording(g_current_tape) && nova_tensor_requires_grad(tensor)) {
        nova_vec_t *inputs = nova_vec_new();
        nova_vec_t *outputs = nova_vec_new();
        
        nova_vec_push(inputs, (void*)tensor);
        nova_vec_push(outputs, (void*)result);
        
        nova_grad_fn_t *grad_fn = nova_grad_fn_create(NOVA_GRAD_OP_TANH, inputs, outputs);
        result->grad_fn = grad_fn;
        result->requires_grad = true;
        
        nova_vec_destroy(inputs);
        nova_vec_destroy(outputs);
    }
    
    return result;
}

nova_tensor_t *nova_tensor_sum_dim(const nova_tensor_t *tensor, size_t dim, bool keepdim)
{
    if (!tensor || dim >= tensor->ndim) return NULL;
    
    // Calculate result shape
    size_t result_ndim = keepdim ? tensor->ndim : tensor->ndim - 1;
    size_t *result_shape = nova_alloc(result_ndim * sizeof(size_t));
    if (!result_shape) return NULL;
    
    size_t result_idx = 0;
    for (size_t i = 0; i < tensor->ndim; i++) {
        if (i != dim) {
            result_shape[result_idx++] = tensor->shape[i];
        } else if (keepdim) {
            result_shape[result_idx++] = 1;
        }
    }
    
    nova_tensor_t *result = nova_tensor_create(result_shape, result_ndim, tensor->dtype);
    nova_free(result_shape);
    
    if (!result) return NULL;
    
    // For now, only handle simple cases
    if (tensor->ndim == 2 && dim == 1) {
        for (size_t i = 0; i < tensor->shape[0]; i++) {
            float sum = 0.0f;
            for (size_t j = 0; j < tensor->shape[1]; j++) {
                sum += ((float*)tensor->data)[i * tensor->shape[1] + j];
            }
            ((float*)result->data)[i] = sum;
        }
    } else {
        // TODO: Implement general dimensional reduction
        nova_tensor_destroy(result);
        return NULL;
    }
    
    return result;
}

nova_tensor_t *nova_tensor_mean_dim(const nova_tensor_t *tensor, size_t dim, bool keepdim)
{
    nova_tensor_t *sum_result = nova_tensor_sum_dim(tensor, dim, keepdim);
    if (!sum_result) return NULL;
    
    // Divide by the size of the dimension being reduced
    float divisor = (float)tensor->shape[dim];
    nova_tensor_t *result = nova_tensor_mul_scalar(sum_result, 1.0f / divisor);
    
    nova_tensor_destroy(sum_result);
    return result;
}

nova_tensor_t *nova_tensor_to_device(nova_tensor_t *tensor, nova_device_t device)
{
    if (!tensor) return NULL;
    
    // For now, only CPU is supported
    if (device != NOVA_DEVICE_CPU) return NULL;
    
    tensor->device = device;
    return tensor;
}

nova_tensor_t *nova_tensor_cpu(nova_tensor_t *tensor)
{
    return nova_tensor_to_device(tensor, NOVA_DEVICE_CPU);
}

nova_tensor_t *nova_tensor_gpu(nova_tensor_t *tensor)
{
    // GPU not implemented yet
    return NULL;
}
