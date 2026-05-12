/**
 * nova_backend_dispatch.h - Unified GPU/CPU Backend Dispatcher
 *
 * Auto-detects best available backend and dispatches operations.
 * Priority: CUDA > Metal > ROCm > Vulkan > OpenCL > CPU
 *
 * Fallback: If the active backend returns an error (ret < 0), each
 * dispatch_* call automatically falls back to CPU.
 */
#ifndef NOVA_BACKEND_DISPATCH_H
#define NOVA_BACKEND_DISPATCH_H

#define NOVA_BACKEND_TYPE_ALREADY_DEFINED 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NOVA_BACKEND_AUTO = 0, // Auto-select best
    NOVA_BACKEND_CPU,
    NOVA_BACKEND_CUDA,
    NOVA_BACKEND_METAL,
    NOVA_BACKEND_ROCM,
    NOVA_BACKEND_VULKAN,
    NOVA_BACKEND_OPENCL,
    NOVA_BACKEND_GPU_ARMY, // 🦅 Specialized Tiered Army Backend (4LUA)
    NOVA_BACKEND_COUNT
} NovaBackendType;

typedef struct {
    NovaBackendType active;
    bool cuda_available;
    bool metal_available;
    bool rocm_available;
    bool vulkan_available;
    bool opencl_available;
    char device_name[256];
} NovaBackendStatus;

// Initialize backend system (probes all available backends)
int nova_backend_init(NovaBackendType preferred);

// Get current backend status
NovaBackendStatus nova_backend_status(void);

// Get name of backend type
const char *nova_backend_name(NovaBackendType type);

// Cleanup all backend resources
void nova_backend_cleanup(void);

// Print available backends
void nova_backend_print_all(void);

// ═══════════════════════════════════════════════════════════════════════════
// Unified Tensor Operations (auto-dispatched to best backend)
// ═══════════════════════════════════════════════════════════════════════════

int64_t nova_dispatch_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n,
                             int64_t k);
int64_t nova_dispatch_add(const float *a, const float *b, float *c, int64_t n);
int64_t nova_dispatch_mul(const float *a, const float *b, float *c, int64_t n);
int64_t nova_dispatch_relu(const float *in, float *out, int64_t n);
int64_t nova_dispatch_softmax(const float *in, float *out, int64_t n);
int64_t nova_dispatch_flash_attention(const float *Q, const float *K, const float *V, float *Out,
                                      int L, int D);

#endif // NOVA_BACKEND_DISPATCH_H
