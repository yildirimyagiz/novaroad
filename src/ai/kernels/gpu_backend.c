/**
 * @file gpu_backend.c
 * @brief Unified GPU Backend Implementation
 */

#include "gpu_backend.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declarations for backend-specific functions
#ifdef NOVA_USE_CUDA
extern void nova_cuda_init_impl(void);
extern void nova_cuda_cleanup_impl(void);
extern bool nova_cuda_is_available_impl(void);
// ... CUDA function declarations
#endif

#ifdef NOVA_USE_METAL
extern void nova_metal_init_impl(void);
extern void nova_metal_cleanup_impl(void);
extern bool nova_metal_is_available_impl(void);
// ... Metal function declarations
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Device Structure
// ═══════════════════════════════════════════════════════════════════════════

struct NovaGPUDevice {
    NovaGPUBackendType backend;
    void* backend_handle;  // Backend-specific context
    int device_id;
    char device_name[256];
};

struct NovaGPUBuffer {
    NovaGPUDevice* device;
    void* ptr;
    size_t size;
};

// ═══════════════════════════════════════════════════════════════════════════
// Device Management
// ═══════════════════════════════════════════════════════════════════════════

NovaGPUDevice* nova_gpu_init(NovaGPUBackendType backend) {
    NovaGPUDevice* device = (NovaGPUDevice*)malloc(sizeof(NovaGPUDevice));
    if (!device) return NULL;
    
    memset(device, 0, sizeof(NovaGPUDevice));
    
    // Auto-detect best backend
    if (backend == NOVA_GPU_BACKEND_AUTO) {
#ifdef __APPLE__
        backend = NOVA_GPU_BACKEND_METAL;
#elif defined(__CUDA_ARCH__)
        backend = NOVA_GPU_BACKEND_CUDA;
#else
        backend = NOVA_GPU_BACKEND_NONE;
#endif
    }
    
    device->backend = backend;
    
    switch (backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            if (!nova_cuda_is_available_impl()) {
                free(device);
                return NULL;
            }
            nova_cuda_init_impl();
            snprintf(device->device_name, sizeof(device->device_name), "NVIDIA CUDA");
            break;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            if (!nova_metal_is_available_impl()) {
                free(device);
                return NULL;
            }
            nova_metal_init_impl();
            snprintf(device->device_name, sizeof(device->device_name), "Apple Metal");
            break;
#endif
            
        default:
            fprintf(stderr, "GPU backend not available\n");
            free(device);
            return NULL;
    }
    
    return device;
}

void nova_gpu_cleanup(NovaGPUDevice* device) {
    if (!device) return;
    
    switch (device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            nova_cuda_cleanup_impl();
            break;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            nova_metal_cleanup_impl();
            break;
#endif
            
        default:
            break;
    }
    
    free(device);
}

bool nova_gpu_is_available(NovaGPUBackendType backend) {
    switch (backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            return nova_cuda_is_available_impl();
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            return nova_metal_is_available_impl();
#endif
            
        default:
            return false;
    }
}

NovaGPUBackendType nova_gpu_get_backend(NovaGPUDevice* device) {
    return device ? device->backend : NOVA_GPU_BACKEND_NONE;
}

void nova_gpu_print_info(NovaGPUDevice* device) {
    if (!device) {
        printf("No GPU device initialized\n");
        return;
    }
    
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("GPU Device Information\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Backend:     %s\n", device->device_name);
    printf("Device ID:   %d\n", device->device_id);
    printf("═══════════════════════════════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// Memory Management
// ═══════════════════════════════════════════════════════════════════════════

NovaGPUBuffer* nova_gpu_malloc(NovaGPUDevice* device, size_t size) {
    if (!device) return NULL;
    
    NovaGPUBuffer* buffer = (NovaGPUBuffer*)malloc(sizeof(NovaGPUBuffer));
    if (!buffer) return NULL;
    
    buffer->device = device;
    buffer->size = size;
    
    // Backend-specific allocation
    switch (device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            // cudaMalloc(&buffer->ptr, size);
            break;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            // Metal buffer allocation
            break;
#endif
            
        default:
            free(buffer);
            return NULL;
    }
    
    return buffer;
}

void nova_gpu_free(NovaGPUBuffer* buffer) {
    if (!buffer) return;
    
    switch (buffer->device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            // cudaFree(buffer->ptr);
            break;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            // Metal buffer free
            break;
#endif
            
        default:
            break;
    }
    
    free(buffer);
}

// ═══════════════════════════════════════════════════════════════════════════
// Wrapper Functions
// ═══════════════════════════════════════════════════════════════════════════

NovaGPUError nova_gpu_maxpool2d(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch, int channels, int in_h, int in_w,
    int out_h, int out_w, int kernel_size, int stride, int padding
) {
    if (!device) return NOVA_GPU_ERROR_INVALID_DEVICE;
    
    switch (device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            nova_cuda_maxpool2d(input, output, batch, channels, in_h, in_w,
                               out_h, out_w, kernel_size, stride, padding);
            return NOVA_GPU_SUCCESS;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            // Call Metal implementation
            return NOVA_GPU_SUCCESS;
#endif
            
        default:
            return NOVA_GPU_ERROR_NOT_AVAILABLE;
    }
}

NovaGPUError nova_gpu_layernorm(
    NovaGPUDevice* device,
    const float* input, float* output,
    int batch_size, int hidden_dim, float eps
) {
    if (!device) return NOVA_GPU_ERROR_INVALID_DEVICE;
    
    switch (device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            nova_cuda_layernorm(input, output, batch_size, hidden_dim, eps);
            return NOVA_GPU_SUCCESS;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            // Call Metal implementation
            return NOVA_GPU_SUCCESS;
#endif
            
        default:
            return NOVA_GPU_ERROR_NOT_AVAILABLE;
    }
}

NovaGPUError nova_gpu_adamw_step(
    NovaGPUDevice* device,
    float* params, const float* grads, float* m, float* v,
    int n, float lr, float beta1, float beta2, float eps,
    float weight_decay, int step
) {
    if (!device) return NOVA_GPU_ERROR_INVALID_DEVICE;
    
    switch (device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            nova_cuda_adamw_step(params, grads, m, v, n, lr, beta1, beta2,
                                eps, weight_decay, step);
            return NOVA_GPU_SUCCESS;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            // Call Metal implementation
            return NOVA_GPU_SUCCESS;
#endif
            
        default:
            return NOVA_GPU_ERROR_NOT_AVAILABLE;
    }
}

NovaGPUError nova_gpu_synchronize(NovaGPUDevice* device) {
    if (!device) return NOVA_GPU_ERROR_INVALID_DEVICE;
    
    switch (device->backend) {
#ifdef NOVA_USE_CUDA
        case NOVA_GPU_BACKEND_CUDA:
            // cudaDeviceSynchronize();
            return NOVA_GPU_SUCCESS;
#endif
            
#ifdef NOVA_USE_METAL
        case NOVA_GPU_BACKEND_METAL:
            // Metal command buffer wait
            return NOVA_GPU_SUCCESS;
#endif
            
        default:
            return NOVA_GPU_ERROR_NOT_AVAILABLE;
    }
}

// Additional wrapper functions follow the same pattern...
