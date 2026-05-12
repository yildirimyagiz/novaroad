/**
 * nova_rocm.h - AMD ROCm/HIP Backend
 * AMD GPU acceleration via HIP runtime (ROCm)
 * Supports: AMD Radeon RX, Instinct MI series
 */
#ifndef NOVA_ROCM_H
#define NOVA_ROCM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  NOVA_ROCM_SUCCESS = 0,
  NOVA_ROCM_ERROR_NOT_AVAILABLE,
  NOVA_ROCM_ERROR_NO_DEVICE,
  NOVA_ROCM_ERROR_OUT_OF_MEMORY,
  NOVA_ROCM_ERROR_INVALID_VALUE,
  NOVA_ROCM_ERROR_NOT_INITIALIZED,
  NOVA_ROCM_ERROR_LAUNCH_FAILED,
  NOVA_ROCM_ERROR_UNKNOWN = 999
} NovaROCmError;

typedef struct {
  char name[256];
  char arch[64];
  uint64_t global_mem_size;
  uint64_t shared_mem_per_block;
  uint32_t compute_units;
  uint32_t max_threads_per_block;
  uint32_t warp_size;
  uint32_t clock_rate_mhz;
  int pci_bus_id;
  int pci_device_id;
  bool supports_fp64;
  bool supports_fp16;
} NovaROCmDeviceInfo;

// Init & Device Management
bool nova_rocm_is_available(void);
int64_t nova_rocm_init(void);
void nova_rocm_cleanup(void);
int64_t nova_rocm_get_device_count(void);
int64_t nova_rocm_set_device(int device_id);
NovaROCmDeviceInfo nova_rocm_get_device_info(int device_idx);
void nova_rocm_print_info(void);

// Memory Management
int64_t nova_rocm_malloc(size_t size);
void nova_rocm_free(int64_t ptr);
int64_t nova_rocm_memcpy_to_device(int64_t dst, const void *src, size_t size);
int64_t nova_rocm_memcpy_to_host(void *dst, int64_t src, size_t size);
int64_t nova_rocm_memset(int64_t dst, int value, size_t size);

// Synchronization
void nova_rocm_synchronize(void);

// Stream Management
int64_t nova_rocm_stream_create(void);
void nova_rocm_stream_destroy(int64_t stream);
void nova_rocm_stream_synchronize(int64_t stream);

// Tensor Operations
int64_t nova_rocm_matmul(const float *a, const float *b, float *c, int64_t m,
                           int64_t n, int64_t k);
int64_t nova_rocm_add(const float *a, const float *b, float *c, int64_t n);
int64_t nova_rocm_mul(const float *a, const float *b, float *c, int64_t n);
int64_t nova_rocm_relu(const float *input, float *output, int64_t n);
int64_t nova_rocm_softmax(const float *input, float *output, int64_t n);
int64_t nova_rocm_reduce_sum(const float *input, float *output, int64_t n);

// rocBLAS integration
int64_t nova_rocm_gemm(bool transA, bool transB, int64_t m, int64_t n,
                         int64_t k, float alpha, const float *A, int64_t lda,
                         const float *B, int64_t ldb, float beta, float *C,
                         int64_t ldc);

#endif // NOVA_ROCM_H
