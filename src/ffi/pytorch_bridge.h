/**
 * @file pytorch_bridge.h
 * @brief PyTorch C++ API Bridge for Real Benchmarking
 * 
 * This bridge allows Nova to call PyTorch C++ API directly
 * for accurate performance comparison.
 */

#ifndef NOVA_PYTORCH_BRIDGE_H
#define NOVA_PYTORCH_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// PyTorch Handle Types
// ═══════════════════════════════════════════════════════════════════════════

typedef void* PyTorchTensor;
typedef void* PyTorchDevice;

// ═══════════════════════════════════════════════════════════════════════════
// Initialization & Cleanup
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Initialize PyTorch
 */
bool pytorch_init(void);

/**
 * Cleanup PyTorch
 */
void pytorch_cleanup(void);

/**
 * Set number of threads
 */
void pytorch_set_num_threads(int num_threads);

/**
 * Check if CUDA is available
 */
bool pytorch_cuda_is_available(void);

/**
 * Get CUDA device count
 */
int pytorch_cuda_device_count(void);

// ═══════════════════════════════════════════════════════════════════════════
// Tensor Creation
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create random tensor
 */
PyTorchTensor pytorch_randn(const size_t* shape, size_t ndim, bool use_cuda);

/**
 * Create zeros tensor
 */
PyTorchTensor pytorch_zeros(const size_t* shape, size_t ndim, bool use_cuda);

/**
 * Create ones tensor
 */
PyTorchTensor pytorch_ones(const size_t* shape, size_t ndim, bool use_cuda);

/**
 * Free tensor
 */
void pytorch_tensor_free(PyTorchTensor tensor);

/**
 * Get tensor data (copy to CPU)
 */
void pytorch_tensor_data(PyTorchTensor tensor, float* out_data, size_t size);

// ═══════════════════════════════════════════════════════════════════════════
// Tensor Operations (for benchmarking)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Matrix multiplication
 */
PyTorchTensor pytorch_matmul(PyTorchTensor a, PyTorchTensor b);

/**
 * Conv2D
 */
PyTorchTensor pytorch_conv2d(
    PyTorchTensor input,
    PyTorchTensor weight,
    size_t stride,
    size_t padding
);

/**
 * MaxPool2D
 */
PyTorchTensor pytorch_max_pool2d(
    PyTorchTensor input,
    size_t kernel_size,
    size_t stride,
    size_t padding
);

/**
 * AvgPool2D
 */
PyTorchTensor pytorch_avg_pool2d(
    PyTorchTensor input,
    size_t kernel_size,
    size_t stride,
    size_t padding
);

/**
 * LayerNorm
 */
PyTorchTensor pytorch_layer_norm(
    PyTorchTensor input,
    size_t normalized_shape,
    float eps
);

/**
 * BatchNorm2D
 */
PyTorchTensor pytorch_batch_norm2d(
    PyTorchTensor input,
    PyTorchTensor running_mean,
    PyTorchTensor running_var,
    PyTorchTensor weight,
    PyTorchTensor bias,
    float eps,
    float momentum
);

/**
 * ReLU
 */
PyTorchTensor pytorch_relu(PyTorchTensor input);

/**
 * GELU
 */
PyTorchTensor pytorch_gelu(PyTorchTensor input);

// ═══════════════════════════════════════════════════════════════════════════
// Loss Functions
// ═══════════════════════════════════════════════════════════════════════════

/**
 * CrossEntropyLoss
 */
float pytorch_cross_entropy_loss(PyTorchTensor logits, PyTorchTensor targets);

/**
 * MSE Loss
 */
float pytorch_mse_loss(PyTorchTensor predictions, PyTorchTensor targets);

// ═══════════════════════════════════════════════════════════════════════════
// Optimizer Operations
// ═══════════════════════════════════════════════════════════════════════════

typedef void* PyTorchOptimizer;

/**
 * Create AdamW optimizer
 */
PyTorchOptimizer pytorch_adamw_create(
    PyTorchTensor* params,
    size_t num_params,
    float lr,
    float beta1,
    float beta2,
    float eps,
    float weight_decay
);

/**
 * Optimizer step
 */
void pytorch_optimizer_step(PyTorchOptimizer optimizer);

/**
 * Zero gradients
 */
void pytorch_optimizer_zero_grad(PyTorchOptimizer optimizer);

/**
 * Free optimizer
 */
void pytorch_optimizer_free(PyTorchOptimizer optimizer);

// ═══════════════════════════════════════════════════════════════════════════
// Timing Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Synchronize CUDA (for accurate timing)
 */
void pytorch_cuda_synchronize(void);

/**
 * Start CUDA event recording
 */
void* pytorch_cuda_event_start(void);

/**
 * End CUDA event recording and return elapsed time in ms
 */
float pytorch_cuda_event_elapsed(void* start_event);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_PYTORCH_BRIDGE_H */
