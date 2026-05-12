/**
 * @file tensor.h
 * @brief N-dimensional tensor operations
 */

#ifndef NOVA_TENSOR_H
#define NOVA_TENSOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Tensor Types & Enums
 * ========================================================================== */

typedef enum {
    NOVA_DTYPE_FLOAT32,
    NOVA_DTYPE_FLOAT64,
    NOVA_DTYPE_INT32,
    NOVA_DTYPE_INT64,
    NOVA_DTYPE_UINT8,
    NOVA_DTYPE_INT8,
    NOVA_DTYPE_BOOL,
} nova_dtype_t;

typedef enum {
    NOVA_DEVICE_CPU,
    NOVA_DEVICE_GPU,
    NOVA_DEVICE_METAL,
} nova_device_t;

typedef struct nova_tensor {
    // Core tensor data
    nova_dtype_t dtype;
    nova_device_t device;
    size_t *shape;
    size_t ndim;
    size_t size;  // total number of elements
    void *data;
    
    // Autograd fields
    bool requires_grad;
    struct nova_tensor *grad;  // gradient tensor
    struct nova_grad_fn *grad_fn;  // gradient function for backprop
    uint32_t grad_accum_count;  // reference count for gradient accumulation
} nova_tensor_t;

/* ============================================================================
 * Tensor Creation & Destruction
 * ========================================================================== */

/**
 * @brief Create tensor with given shape and dtype
 * @param shape Array of dimensions
 * @param ndim Number of dimensions
 * @param dtype Data type
 * @return New tensor or NULL on failure
 */
nova_tensor_t *nova_tensor_create(const size_t *shape, size_t ndim, nova_dtype_t dtype);

/**
 * @brief Create tensor filled with zeros
 */
nova_tensor_t *nova_tensor_zeros(const size_t *shape, size_t ndim, nova_dtype_t dtype);

/**
 * @brief Create tensor filled with ones
 */
nova_tensor_t *nova_tensor_ones(const size_t *shape, size_t ndim, nova_dtype_t dtype);

/**
 * @brief Create tensor with random values (uniform [0, 1))
 */
nova_tensor_t *nova_tensor_rand(const size_t *shape, size_t ndim, nova_dtype_t dtype);

/**
 * @brief Create tensor with random normal distribution (mean=0, std=1)
 */
nova_tensor_t *nova_tensor_randn(const size_t *shape, size_t ndim, nova_dtype_t dtype);

/**
 * @brief Create tensor from existing data (copies data)
 */
nova_tensor_t *nova_tensor_from_data(const void *data, const size_t *shape, 
                                      size_t ndim, nova_dtype_t dtype);

/**
 * @brief Create 1D tensor (vector) from array
 */
nova_tensor_t *nova_tensor_from_array_f32(const float *data, size_t len);

/**
 * @brief Clone tensor (deep copy)
 */
nova_tensor_t *nova_tensor_clone(const nova_tensor_t *tensor);

/**
 * @brief Destroy tensor and free memory
 */
void nova_tensor_destroy(nova_tensor_t *tensor);

/* ============================================================================
 * Tensor Properties
 * ========================================================================== */

/**
 * @brief Get number of dimensions
 */
size_t nova_tensor_ndim(const nova_tensor_t *tensor);

/**
 * @brief Get shape array
 */
const size_t *nova_tensor_shape(const nova_tensor_t *tensor);

/**
 * @brief Get size of specific dimension
 */
size_t nova_tensor_size(const nova_tensor_t *tensor, size_t dim);

/**
 * @brief Get total number of elements
 */
size_t nova_tensor_numel(const nova_tensor_t *tensor);

/**
 * @brief Get data type
 */
nova_dtype_t nova_tensor_dtype(const nova_tensor_t *tensor);

/**
 * @brief Get device (CPU/GPU)
 */
nova_device_t nova_tensor_device(const nova_tensor_t *tensor);

/**
 * @brief Get raw data pointer (use with caution)
 */
void *nova_tensor_data(nova_tensor_t *tensor);

/**
 * @brief Get element size in bytes
 */
size_t nova_tensor_element_size(const nova_tensor_t *tensor);

/* ============================================================================
 * Element Access
 * ========================================================================== */

/**
 * @brief Get element as float (converts if needed)
 */
float nova_tensor_get_f32(const nova_tensor_t *tensor, const size_t *indices);

/**
 * @brief Set element from float (converts if needed)
 */
void nova_tensor_set_f32(nova_tensor_t *tensor, const size_t *indices, float value);

/**
 * @brief Get element at flat index
 */
float nova_tensor_get_item(const nova_tensor_t *tensor, size_t index);

/**
 * @brief Set element at flat index
 */
void nova_tensor_set_item(nova_tensor_t *tensor, size_t index, float value);

/* ============================================================================
 * Tensor Reshaping & Manipulation
 * ========================================================================== */

/**
 * @brief Reshape tensor (must preserve total elements)
 */
nova_tensor_t *nova_tensor_reshape(nova_tensor_t *tensor, const size_t *new_shape, size_t new_ndim);

/**
 * @brief Transpose tensor (swap axes)
 */
nova_tensor_t *nova_tensor_transpose(const nova_tensor_t *tensor, size_t dim0, size_t dim1);

/**
 * @brief Permute dimensions
 */
nova_tensor_t *nova_tensor_permute(const nova_tensor_t *tensor, const size_t *dims);

/**
 * @brief Squeeze dimensions of size 1
 */
nova_tensor_t *nova_tensor_squeeze(const nova_tensor_t *tensor);

/**
 * @brief Add dimension of size 1 at position
 */
nova_tensor_t *nova_tensor_unsqueeze(const nova_tensor_t *tensor, size_t dim);

/**
 * @brief Flatten tensor to 1D
 */
nova_tensor_t *nova_tensor_flatten(const nova_tensor_t *tensor);

/**
 * @brief Slice tensor along dimension
 */
nova_tensor_t *nova_tensor_slice(const nova_tensor_t *tensor, size_t dim, 
                                  size_t start, size_t end);

/* ============================================================================
 * Arithmetic Operations
 * ========================================================================== */

/**
 * @brief Element-wise addition: a + b
 */
nova_tensor_t *nova_tensor_add(const nova_tensor_t *a, const nova_tensor_t *b);

/**
 * @brief Element-wise subtraction: a - b
 */
nova_tensor_t *nova_tensor_sub(const nova_tensor_t *a, const nova_tensor_t *b);

/**
 * @brief Element-wise multiplication: a * b
 */
nova_tensor_t *nova_tensor_mul(const nova_tensor_t *a, const nova_tensor_t *b);

/**
 * @brief Element-wise division: a / b
 */
nova_tensor_t *nova_tensor_div(const nova_tensor_t *a, const nova_tensor_t *b);

/**
 * @brief Scalar addition: tensor + scalar
 */
nova_tensor_t *nova_tensor_add_scalar(const nova_tensor_t *tensor, float scalar);

/**
 * @brief Scalar multiplication: tensor * scalar
 */
nova_tensor_t *nova_tensor_mul_scalar(const nova_tensor_t *tensor, float scalar);

/**
 * @brief Matrix multiplication (2D tensors)
 */
nova_tensor_t *nova_tensor_matmul(const nova_tensor_t *a, const nova_tensor_t *b);

/**
 * @brief Dot product (1D tensors)
 */
float nova_tensor_dot(const nova_tensor_t *a, const nova_tensor_t *b);

/* ============================================================================
 * Mathematical Functions
 * ========================================================================== */

/**
 * @brief Element-wise exponential
 */
nova_tensor_t *nova_tensor_exp(const nova_tensor_t *tensor);

/**
 * @brief Element-wise natural logarithm
 */
nova_tensor_t *nova_tensor_log(const nova_tensor_t *tensor);

/**
 * @brief Element-wise square root
 */
nova_tensor_t *nova_tensor_sqrt(const nova_tensor_t *tensor);

/**
 * @brief Element-wise power
 */
nova_tensor_t *nova_tensor_pow(const nova_tensor_t *tensor, float exponent);

/**
 * @brief Element-wise absolute value
 */
nova_tensor_t *nova_tensor_abs(const nova_tensor_t *tensor);

/**
 * @brief Element-wise sine
 */
nova_tensor_t *nova_tensor_sin(const nova_tensor_t *tensor);

/**
 * @brief Element-wise cosine
 */
nova_tensor_t *nova_tensor_cos(const nova_tensor_t *tensor);

/**
 * @brief Element-wise tanh
 */
nova_tensor_t *nova_tensor_tanh(const nova_tensor_t *tensor);

/* ============================================================================
 * Reduction Operations
 * ========================================================================== */

/**
 * @brief Sum all elements
 */
float nova_tensor_sum(const nova_tensor_t *tensor);

/**
 * @brief Sum along dimension
 */
nova_tensor_t *nova_tensor_sum_dim(const nova_tensor_t *tensor, size_t dim, bool keepdim);

/**
 * @brief Mean of all elements
 */
float nova_tensor_mean(const nova_tensor_t *tensor);

/**
 * @brief Mean along dimension
 */
nova_tensor_t *nova_tensor_mean_dim(const nova_tensor_t *tensor, size_t dim, bool keepdim);

/**
 * @brief Maximum element
 */
float nova_tensor_max(const nova_tensor_t *tensor);

/**
 * @brief Minimum element
 */
float nova_tensor_min(const nova_tensor_t *tensor);

/**
 * @brief Index of maximum element
 */
size_t nova_tensor_argmax(const nova_tensor_t *tensor);

/**
 * @brief Index of minimum element
 */
size_t nova_tensor_argmin(const nova_tensor_t *tensor);

/* ============================================================================
 * Device Management
 * ========================================================================== */

/**
 * @brief Move tensor to device (CPU/GPU)
 */
nova_tensor_t *nova_tensor_to_device(nova_tensor_t *tensor, nova_device_t device);

/**
 * @brief Move tensor to CPU
 */
nova_tensor_t *nova_tensor_cpu(nova_tensor_t *tensor);

/**
 * @brief Move tensor to GPU
 */
nova_tensor_t *nova_tensor_gpu(nova_tensor_t *tensor);

/* ============================================================================
 * Utilities
 * ========================================================================== */

/**
 * @brief Print tensor info and values
 */
void nova_tensor_print(const nova_tensor_t *tensor);

/**
 * @brief Check if tensors have same shape
 */
bool nova_tensor_same_shape(const nova_tensor_t *a, const nova_tensor_t *b);

/**
 * @brief Convert dtype to string
 */
const char *nova_dtype_to_string(nova_dtype_t dtype);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_TENSOR_H */
