/**
 * @file gpu_backend.h
 * @brief Unified GPU Backend Interface
 * 
 * Provides a unified API for GPU operations across:
 * - CUDA (NVIDIA)
 * - Metal (Apple)
 * - ROCm (AMD)
 * - Vulkan (Cross-platform)
 */

#ifndef NOVA_GPU_BACKEND_H
#define NOVA_GPU_BACKEND_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════
// GPU Backend Types
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_GPU_BACKEND_NONE = 0,
    NOVA_GPU_BACKEND_CUDA,
    NOVA_GPU_BACKEND_METAL,
    NOVA_GPU_BACKEND_ROCM,
    NOVA_GPU_BACKEND_VULKAN,
    NOVA_GPU_BACKEND_AUTO  // Auto-detect best available
} NovaGPUBackendType;

typedef enum {
    NOVA_GPU_SUCCESS = 0,
    NOVA_GPU_ERROR_NOT_AVAILABLE,
    NOVA_GPU_ERROR_OUT_OF_MEMORY,
    NOVA_GPU_ERROR_INVALID_DEVICE,
    NOVA_GPU_ERROR_KERNEL_LAUNCH_FAILED,
    NOVA_GPU_ERROR_SYNC_FAILED
} NovaGPUError;

typedef struct NovaGPUDevice NovaGPUDevice;
typedef struct NovaGPUBuffer NovaGPUBuffer;

// ═══════════════════════════════════════════════════════════════════════════
// Device Management
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Initialize GPU backend
 * @param backend Backend type (or AUTO for auto-detection)
 * @return Device handle or NULL on failure
 */
NovaGPUDevice* nova_gpu_init(NovaGPUBackendType backend);

/**
 * Cleanup GPU resources
 */
void nova_gpu_cleanup(NovaGPUDevice* device);

/**
 * Check if GPU backend is available
 */
bool nova_gpu_is_available(NovaGPUBackendType backend);

/**
 * Get current backend type
 */
NovaGPUBackendType nova_gpu_get_backend(NovaGPUDevice* device);

/**
 * Print GPU device info
 */
void nova_gpu_print_info(NovaGPUDevice* device);

// ═══════════════════════════════════════════════════════════════════════════
// Memory Management
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Allocate GPU memory
 */
NovaGPUBuffer* nova_gpu_malloc(NovaGPUDevice* device, size_t size);

/**
 * Free GPU memory
 */
void nova_gpu_free(NovaGPUBuffer* buffer);

/**
 * Copy data to GPU
 */
NovaGPUError nova_gpu_memcpy_h2d(NovaGPUBuffer* dst, const void* src, size_t size);

/**
 * Copy data from GPU
 */
NovaGPUError nova_gpu_memcpy_d2h(void* dst, NovaGPUBuffer* src, size_t size);

/**
 * Get raw pointer (device-side)
 */
void* nova_gpu_get_ptr(NovaGPUBuffer* buffer);

// ═══════════════════════════════════════════════════════════════════════════
// Neural Network Operations
// ═══════════════════════════════════════════════════════════════════════════

// Pooling Operations
NovaGPUError nova_gpu_maxpool2d(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch, int channels, int in_h, int in_w,
    int out_h, int out_w, int kernel_size, int stride, int padding
);

NovaGPUError nova_gpu_avgpool2d(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch, int channels, int in_h, int in_w,
    int out_h, int out_w, int kernel_size, int stride, int padding
);

NovaGPUError nova_gpu_global_avgpool(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch, int channels, int height, int width
);

// Normalization Operations
NovaGPUError nova_gpu_layernorm(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch_size, int hidden_dim, float eps
);

NovaGPUError nova_gpu_batchnorm(
    NovaGPUDevice* device,
    const float* input, float* output,
    const float* mean, const float* variance,
    const float* gamma, const float* beta,
    int batch_size, int num_channels, int spatial_size, float eps
);

NovaGPUError nova_gpu_rmsnorm(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch_size, int hidden_dim, float eps
);

// Loss Functions
NovaGPUError nova_gpu_cross_entropy(
    NovaGPUDevice* device,
    const float* logits, const float* targets,
    float* loss, int batch_size, int num_classes
);

NovaGPUError nova_gpu_mse_loss(
    NovaGPUDevice* device,
    const float* predictions, const float* targets,
    float* loss, int n
);

// Optimizer Operations
NovaGPUError nova_gpu_adamw_step(
    NovaGPUDevice* device,
    float* params, const float* grads, float* m, float* v,
    int n, float lr, float beta1, float beta2, float eps,
    float weight_decay, int step
);

NovaGPUError nova_gpu_sgd_step(
    NovaGPUDevice* device,
    float* params, const float* grads, float* velocity,
    int n, float lr, float momentum, float weight_decay, bool nesterov
);

// Basic Operations
NovaGPUError nova_gpu_matmul(
    NovaGPUDevice* device,
    const float* A, const float* B, float* C,
    int M, int N, int K
);

NovaGPUError nova_gpu_relu(
    NovaGPUDevice* device,
    const float* input, float* output, int n
);

NovaGPUError nova_gpu_gelu(
    NovaGPUDevice* device,
    const float* input, float* output, int n
);

// ═══════════════════════════════════════════════════════════════════════════
// Synchronization
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Synchronize GPU operations
 */
NovaGPUError nova_gpu_synchronize(NovaGPUDevice* device);

#ifdef __cplusplus
}
#endif

#endif // NOVA_GPU_BACKEND_H
