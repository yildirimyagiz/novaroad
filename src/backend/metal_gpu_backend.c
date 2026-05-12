/**
 * @file metal_gpu_backend.c
 * @brief Metal GPU backend wrapper for Nova
 * 
 * Provides easy-to-use C API for Metal GPU acceleration
 * Targets: 100-300x speedup for large matrices on M1 GPU
 */

#ifdef __APPLE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for Objective-C Metal interface
// Implementation in metal_gpu_backend.m (Objective-C++)
extern void* metal_device_create(void);
extern void metal_device_destroy(void* device);
extern int metal_gemm_execute(void* device,
                              const float* A, const float* B, float* C,
                              int M, int N, int K);
extern int metal_gemm_tiled_execute(void* device,
                                    const float* A, const float* B, float* C,
                                    int M, int N, int K);
extern int metal_gemm_fp16_execute(void* device,
                                   const void* A, const void* B, void* C,
                                   int M, int N, int K);

// Global Metal device (initialized on first use)
static void* g_metal_device = NULL;

/**
 * Initialize Metal GPU backend
 */
int nova_metal_gpu_init(void)
{
    if (g_metal_device) return 0; // Already initialized
    
    g_metal_device = metal_device_create();
    if (!g_metal_device) {
        fprintf(stderr, "Failed to create Metal device\n");
        return -1;
    }
    
    printf("✅ Nova Metal GPU initialized\n");
    return 0;
}

/**
 * Cleanup Metal GPU backend
 */
void nova_metal_gpu_cleanup(void)
{
    if (g_metal_device) {
        metal_device_destroy(g_metal_device);
        g_metal_device = NULL;
    }
}

/**
 * GPU-accelerated matrix multiplication
 * Automatically selects best kernel based on size
 */
int nova_metal_gpu_gemm(
    const float* A,  // [M×K]
    const float* B,  // [K×N]
    float* C,        // [M×N]
    int M, int N, int K)
{
    if (!g_metal_device) {
        if (nova_metal_gpu_init() != 0) return -1;
    }
    
    // Choose kernel based on matrix size
    if (M * N * K < 64*64*64) {
        // Small matrices: use simple kernel
        return metal_gemm_execute(g_metal_device, A, B, C, M, N, K);
    } else {
        // Large matrices: use tiled kernel for better cache reuse
        return metal_gemm_tiled_execute(g_metal_device, A, B, C, M, N, K);
    }
}

/**
 * FP16 GPU GEMM (2x faster)
 */
int nova_metal_gpu_gemm_fp16(
    const void* A,
    const void* B,
    void* C,
    int M, int N, int K)
{
    if (!g_metal_device) {
        if (nova_metal_gpu_init() != 0) return -1;
    }
    
    return metal_gemm_fp16_execute(g_metal_device, A, B, C, M, N, K);
}

#else /* !__APPLE__ */

int nova_metal_gpu_init(void) { return -1; }
void nova_metal_gpu_cleanup(void) {}
int nova_metal_gpu_gemm(const float* A, const float* B, float* C,
                        int M, int N, int K) {
    (void)A; (void)B; (void)C; (void)M; (void)N; (void)K;
    return -1;
}

#endif /* __APPLE__ */
