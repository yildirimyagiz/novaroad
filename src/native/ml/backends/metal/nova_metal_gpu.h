/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_gpu.h - GPU Acceleration (Mojo'nun CUDA/Metal Desteği)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_GPU_H
#define NOVA_GPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// GPU backend selection
typedef enum {
  NOVA_GPU_CUDA,   // NVIDIA CUDA
  NOVA_GPU_METAL,  // Apple Metal
  NOVA_GPU_VULKAN, // Vulkan Compute
  NOVA_GPU_OPENCL, // OpenCL
  NOVA_GPU_ROCM,   // AMD ROCm
} NovaGPUBackend;

typedef struct NovaGPUContext NovaGPUContext;
typedef struct NovaGPUBuffer NovaGPUBuffer;
typedef struct NovaGPUKernel NovaGPUKernel;

// GPU context management
NovaGPUContext *nova_gpu_create(NovaGPUBackend backend);
void nova_gpu_destroy(NovaGPUContext *ctx);
bool nova_gpu_is_available(NovaGPUBackend backend);

// Buffer operations (unified memory)
NovaGPUBuffer *nova_gpu_buffer_create(NovaGPUContext *ctx, size_t size,
                                          bool host_visible);
void nova_gpu_buffer_destroy(NovaGPUBuffer *buf);
void *nova_gpu_buffer_map(NovaGPUBuffer *buf);
void nova_gpu_buffer_unmap(NovaGPUBuffer *buf);
void nova_gpu_buffer_copy_to_host(NovaGPUBuffer *buf, void *host_ptr,
                                    size_t size);
void nova_gpu_buffer_copy_to_device(NovaGPUBuffer *buf,
                                      const void *host_ptr, size_t size);

// Kernel compilation and execution
NovaGPUKernel *nova_gpu_kernel_compile(NovaGPUContext *ctx,
                                           const char *source,
                                           const char *entry_point);
void nova_gpu_kernel_destroy(NovaGPUKernel *kernel);
void nova_gpu_kernel_set_arg(NovaGPUKernel *kernel, uint32_t index,
                               const void *arg, size_t size);
void nova_gpu_kernel_launch(NovaGPUKernel *kernel, size_t global_size[3],
                              size_t local_size[3]);
void nova_gpu_synchronize(NovaGPUContext *ctx);

// High-level operations (auto-dispatch to GPU)
void nova_gpu_matmul_f32(NovaGPUContext *ctx, const float *A,
                           const float *B, float *C, size_t M, size_t N,
                           size_t K);
void nova_gpu_reduce_sum_f32(NovaGPUContext *ctx, const float *input,
                               float *output, size_t n);
void nova_gpu_vector_add_f32(NovaGPUContext *ctx, const float *a,
                               const float *b, float *c, size_t n);

// Native Metal Interface
int64_t nova_metal_init(void);
void nova_metal_cleanup(void);
int64_t nova_metal_matmul(const float *a, const float *b, float *c, int64_t m,
                            int64_t n, int64_t k);
int64_t nova_metal_add(const float *a, const float *b, float *c, int64_t count);
int64_t nova_metal_mul(const float *a, const float *b, float *c, int64_t count);
int64_t nova_metal_relu(const float *in, float *out, int64_t n);
int64_t nova_metal_softmax(const float *in, float *out, int64_t n);
void nova_metal_print_info(void);

#endif // NOVA_GPU_H
