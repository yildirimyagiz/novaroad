/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_cuda.h - CUDA Acceleration Backend
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_CUDA_H
#define NOVA_CUDA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// CUDA Error Codes
typedef enum {
  NOVA_CUDA_SUCCESS = 0,
  NOVA_CUDA_ERROR_INVALID_VALUE,
  NOVA_CUDA_ERROR_OUT_OF_MEMORY,
  NOVA_CUDA_ERROR_NOT_INITIALIZED,
  NOVA_CUDA_ERROR_DEINITIALIZED,
  NOVA_CUDA_ERROR_NO_DEVICE,
  NOVA_CUDA_ERROR_INVALID_DEVICE,
  NOVA_CUDA_ERROR_INVALID_IMAGE,
  NOVA_CUDA_ERROR_INVALID_CONTEXT,
  NOVA_CUDA_ERROR_MAP_FAILED,
  NOVA_CUDA_ERROR_UNMAP_FAILED,
  NOVA_CUDA_ERROR_ARRAY_IS_MAPPED,
  NOVA_CUDA_ERROR_ALREADY_MAPPED,
  NOVA_CUDA_ERROR_NO_BINARY_FOR_GPU,
  NOVA_CUDA_ERROR_ALREADY_ACQUIRED,
  NOVA_CUDA_ERROR_NOT_MAPPED,
  NOVA_CUDA_ERROR_NOT_MAPPED_AS_ARRAY,
  NOVA_CUDA_ERROR_NOT_MAPPED_AS_POINTER,
  NOVA_CUDA_ERROR_ECC_UNCORRECTABLE,
  NOVA_CUDA_ERROR_UNSUPPORTED_LIMIT,
  NOVA_CUDA_ERROR_CONTEXT_ALREADY_IN_USE,
  NOVA_CUDA_ERROR_PEER_ACCESS_UNSUPPORTED,
  NOVA_CUDA_ERROR_INVALID_PTX,
  NOVA_CUDA_ERROR_INVALID_GRAPHICS_CONTEXT,
  NOVA_CUDA_ERROR_NVLINK_UNCORRECTABLE,
  NOVA_CUDA_ERROR_JIT_COMPILER_NOT_FOUND,
  NOVA_CUDA_ERROR_UNSUPPORTED_PTX_VERSION,
  NOVA_CUDA_ERROR_UNKNOWN = 999
} NovaCUDAError;

// CUDA Backend Interface
int64_t nova_cuda_init(void);
void nova_cuda_cleanup(void);
int64_t nova_cuda_get_device_count(void);
void nova_cuda_print_info(void);

// Memory Management
int64_t nova_cuda_malloc(size_t size);
void nova_cuda_free(int64_t ptr);
int64_t nova_cuda_memcpy_to_device(int64_t dst, const void *src, size_t size);
int64_t nova_cuda_memcpy_to_host(void *dst, int64_t src, size_t size);

// Operations
int64_t nova_cuda_matmul(const float *a, const float *b, float *c, int64_t m,
                           int64_t n, int64_t k);
int64_t nova_cuda_add(const float *a, const float *b, float *c, int64_t n);
int64_t nova_cuda_mul(const float *a, const float *b, float *c, int64_t n);

#endif // NOVA_CUDA_H
