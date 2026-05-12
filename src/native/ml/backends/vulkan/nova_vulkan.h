/**
 * nova_vulkan.h - Vulkan Compute Backend
 *
 * Cross-platform GPU compute via Vulkan
 * Supports: NVIDIA, AMD, Intel, Mobile (Adreno, Mali)
 */
#ifndef NOVA_VULKAN_H
#define NOVA_VULKAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  NOVA_VK_SUCCESS = 0,
  NOVA_VK_ERROR_NOT_AVAILABLE,
  NOVA_VK_ERROR_NO_DEVICE,
  NOVA_VK_ERROR_INIT_FAILED,
  NOVA_VK_ERROR_ALLOC_FAILED,
  NOVA_VK_ERROR_PIPELINE_FAILED,
  NOVA_VK_ERROR_SHADER_FAILED,
  NOVA_VK_ERROR_UNKNOWN = 999
} NovaVKError;

typedef struct {
  char name[256];
  char driver_version[64];
  uint32_t api_version;
  uint64_t vram_size;
  uint32_t compute_queues;
  uint32_t max_work_group_size;
  uint32_t max_workgroup_invocations;
  bool supports_fp64;
  bool supports_fp16;
  bool supports_int8;
  bool is_discrete;
} NovaVKDeviceInfo;

// Init & Device
bool nova_vk_is_available(void);
int64_t nova_vk_init(void);
void nova_vk_cleanup(void);
int64_t nova_vk_get_device_count(void);
NovaVKDeviceInfo nova_vk_get_device_info(int idx);
void nova_vk_print_info(void);

// Memory
int64_t nova_vk_malloc(size_t size);
void nova_vk_free(int64_t handle);
int64_t nova_vk_memcpy_to_device(int64_t dst, const void *src, size_t size);
int64_t nova_vk_memcpy_to_host(void *dst, int64_t src, size_t size);

// Compute
int64_t nova_vk_matmul(const float *a, const float *b, float *c, int64_t m,
                         int64_t n, int64_t k);
int64_t nova_vk_add(const float *a, const float *b, float *c, int64_t n);
int64_t nova_vk_mul(const float *a, const float *b, float *c, int64_t n);
int64_t nova_vk_relu(const float *input, float *output, int64_t n);
void nova_vk_synchronize(void);

#endif // NOVA_VULKAN_H
