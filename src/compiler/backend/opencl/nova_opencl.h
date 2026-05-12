/**
 * nova_opencl.h - OpenCL Compute Backend
 * Cross-platform GPU: Intel, AMD, NVIDIA, ARM Mali, Qualcomm Adreno
 */
#ifndef NOVA_OPENCL_H
#define NOVA_OPENCL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  NOVA_CL_SUCCESS = 0,
  NOVA_CL_ERROR_NOT_AVAILABLE,
  NOVA_CL_ERROR_NO_PLATFORM,
  NOVA_CL_ERROR_NO_DEVICE,
  NOVA_CL_ERROR_CONTEXT_FAILED,
  NOVA_CL_ERROR_QUEUE_FAILED,
  NOVA_CL_ERROR_ALLOC_FAILED,
  NOVA_CL_ERROR_MEMCPY_FAILED,
  NOVA_CL_ERROR_COMPILE_FAILED,
  NOVA_CL_ERROR_KERNEL_FAILED,
  NOVA_CL_ERROR_LAUNCH_FAILED,
  NOVA_CL_ERROR_UNKNOWN = 999
} NovaCLError;

typedef enum {
  NOVA_CL_DEVICE_GPU = 0,
  NOVA_CL_DEVICE_CPU,
  NOVA_CL_DEVICE_ACCELERATOR
} NovaCLDeviceType;

typedef struct {
  char name[256];
  char vendor[128];
  char driver_version[64];
  NovaCLDeviceType type;
  uint64_t global_mem_size;
  uint64_t local_mem_size;
  uint32_t max_compute_units;
  uint32_t max_work_group_size;
  bool supports_fp64;
  bool supports_fp16;
} NovaCLDeviceInfo;

typedef struct NovaCLProgram NovaCLProgram;
typedef struct NovaCLKernel NovaCLKernel;

// Init & Device
bool nova_cl_is_available(void);
int64_t nova_cl_init(void);
int64_t nova_cl_init_with_device(NovaCLDeviceType preferred);
void nova_cl_cleanup(void);
int64_t nova_cl_get_platform_count(void);
int64_t nova_cl_get_device_count(void);
NovaCLDeviceInfo nova_cl_get_device_info(int device_idx);
void nova_cl_print_info(void);

// Memory
int64_t nova_cl_malloc(size_t size);
void nova_cl_free(int64_t handle);
int64_t nova_cl_memcpy_to_device(int64_t dst, const void *src, size_t size);
int64_t nova_cl_memcpy_to_host(void *dst, int64_t src, size_t size);

// Kernels
NovaCLProgram *nova_cl_compile_program(const char *source,
                                           const char *opts);
NovaCLKernel *nova_cl_get_kernel(NovaCLProgram *prog, const char *name);
int64_t nova_cl_set_kernel_arg(NovaCLKernel *k, uint32_t idx, size_t sz,
                                 const void *val);
int64_t nova_cl_launch_kernel(NovaCLKernel *k, uint32_t dim,
                                const size_t *global, const size_t *local);
void nova_cl_synchronize(void);
void nova_cl_destroy_program(NovaCLProgram *prog);
void nova_cl_destroy_kernel(NovaCLKernel *k);

// Tensor Ops
int64_t nova_cl_matmul(const float *a, const float *b, float *c, int64_t m,
                         int64_t n, int64_t k);
int64_t nova_cl_add(const float *a, const float *b, float *c, int64_t n);
int64_t nova_cl_mul(const float *a, const float *b, float *c, int64_t n);
int64_t nova_cl_relu(const float *input, float *output, int64_t n);
int64_t nova_cl_softmax(const float *input, float *output, int64_t n);
int64_t nova_cl_reduce_sum(const float *input, float *output, int64_t n);

// Built-in kernel sources
const char *nova_cl_kernel_matmul_source(void);
const char *nova_cl_kernel_elementwise_source(void);
const char *nova_cl_kernel_activation_source(void);

#endif // NOVA_OPENCL_H
