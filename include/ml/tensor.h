// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA TENSOR BACKEND  v2.0                                             ║
// ║  C implementation of multi-dimensional arrays with autograd             ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_ML_TENSOR_H
#define NOVA_ML_TENSOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ══════════════════════════════════════════════════════════════════════════════
// DATA TYPES
// ══════════════════════════════════════════════════════════════════════════════

typedef enum nova_dtype {
    NOVA_DTYPE_F32,
    NOVA_DTYPE_F64,
    NOVA_DTYPE_I32,
    NOVA_DTYPE_I64
} nova_dtype_t;

// Gradient function types
typedef enum nova_grad_fn_type {
    NOVA_GRAD_FN_ADD,
    NOVA_GRAD_FN_MATMUL,
    NOVA_GRAD_FN_MAX
} nova_grad_fn_type_t;

// Forward declarations
typedef struct nova_tensor nova_tensor_t;
typedef struct nova_grad_fn nova_grad_fn_t;

// ══════════════════════════════════════════════════════════════════════════════
// STRUCT DEFINITIONS (Moved to header to allow access from tape.c)
// ══════════════════════════════════════════════════════════════════════════════

struct nova_grad_fn {
    nova_grad_fn_type_t type;
    nova_tensor_t *input1;
    nova_tensor_t *input2;
    void (*backward)(nova_grad_fn_t *fn, nova_tensor_t *grad_output, nova_tensor_t **grad_input1,
                     nova_tensor_t **grad_input2);
};

struct nova_tensor {
    nova_dtype_t dtype;
    size_t ndims;
    size_t *shape;
    size_t *strides;
    void *data;
    size_t data_size;
    bool requires_grad;
    nova_grad_fn_t *grad_fn;
    struct nova_tensor *grad;
    char *name; // For debugging
};

// ══════════════════════════════════════════════════════════════════════════════
// TENSOR LIFECYCLE
// ══════════════════════════════════════════════════════════════════════════════

nova_tensor_t *nova_tensor_create(nova_dtype_t dtype, size_t ndims, const size_t *shape);
nova_tensor_t *nova_tensor_zeros(nova_dtype_t dtype, size_t ndims, const size_t *shape);
nova_tensor_t *nova_tensor_ones(nova_dtype_t dtype, size_t ndims, const size_t *shape);
void nova_tensor_destroy(nova_tensor_t *tensor);
nova_tensor_t *nova_tensor_clone(const nova_tensor_t *tensor);

// ══════════════════════════════════════════════════════════════════════════════
// TENSOR OPERATIONS
// ══════════════════════════════════════════════════════════════════════════════

nova_tensor_t *nova_tensor_add(const nova_tensor_t *a, const nova_tensor_t *b);
nova_tensor_t *nova_tensor_matmul(const nova_tensor_t *a, const nova_tensor_t *b);
nova_tensor_t *nova_tensor_transpose(const nova_tensor_t *tensor);

// Comparison and utility
bool nova_tensor_shapes_equal(const nova_tensor_t *a, const nova_tensor_t *b);
size_t nova_dtype_size(nova_dtype_t dtype);

// ══════════════════════════════════════════════════════════════════════════════
// TENSOR DATA ACCESS
// ══════════════════════════════════════════════════════════════════════════════

float nova_tensor_get_f32(const nova_tensor_t *tensor, const size_t *indices);
void nova_tensor_set_f32(nova_tensor_t *tensor, const size_t *indices, float value);

// ══════════════════════════════════════════════════════════════════════════════
// AUTOGRAD INTERFACE
// ══════════════════════════════════════════════════════════════════════════════

int nova_tensor_backward(nova_tensor_t *tensor);
int nova_tensor_backward_with_grad(nova_tensor_t *tensor, nova_tensor_t *grad_output);

// Gradient function management
void nova_grad_fn_destroy(nova_grad_fn_t *fn);
nova_grad_fn_t *nova_grad_fn_add(const nova_tensor_t *a, const nova_tensor_t *b);
nova_grad_fn_t *nova_grad_fn_matmul(const nova_tensor_t *a, const nova_tensor_t *b);
nova_grad_fn_t *nova_grad_fn_clone(nova_grad_fn_t *fn);

// Backward implementation types
void nova_grad_fn_add_backward(nova_grad_fn_t *fn, nova_tensor_t *grad_output,
                               nova_tensor_t **grad_input1, nova_tensor_t **grad_input2);
void nova_grad_fn_matmul_backward(nova_grad_fn_t *fn, nova_tensor_t *grad_output,
                                  nova_tensor_t **grad_input1, nova_tensor_t **grad_input2);

#endif /* NOVA_ML_TENSOR_H */
