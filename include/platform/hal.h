/**
 * @file hal.h
 * @brief Hardware Abstraction Layer (HAL) - Cross-platform hardware interface
 */

#ifndef NOVA_HAL_H
#define NOVA_HAL_H

#include "../config/config.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device types */
typedef enum {
    NOVA_DEVICE_TYPE_CPU,
    NOVA_DEVICE_TYPE_GPU,
    NOVA_DEVICE_TYPE_TPU,
    NOVA_DEVICE_TYPE_DSP,
    NOVA_DEVICE_TYPE_NPU,
} nova_device_type_t;

/* Memory types */
typedef enum {
    NOVA_MEMORY_TYPE_HOST,
    NOVA_MEMORY_TYPE_DEVICE,
    NOVA_MEMORY_TYPE_UNIFIED,
    NOVA_MEMORY_TYPE_PINNED,
} nova_memory_type_t;

/* Device capabilities */
typedef struct {
    nova_device_type_t type;
    const char *name;
    const char *vendor;
    size_t total_memory;
    size_t compute_units;
    bool supports_fp64;
    bool supports_fp32;
    bool supports_fp16;
    bool supports_int64;
    bool supports_int32;
    bool supports_int16;
    bool supports_int8;
    bool supports_async;
    bool supports_unified_memory;
} nova_device_caps_t;

/* Memory allocation handle */
typedef struct nova_memory_handle nova_memory_handle_t;

/* Device handle */
typedef struct nova_device_handle nova_device_handle_t;

/* HAL context */
typedef struct nova_hal_context nova_hal_context_t;

/* ============================================================================
 * HAL Context Management
 * ========================================================================== */

/**
 * Initialize HAL
 */
int nova_hal_init(void);

/**
 * Shutdown HAL
 */
void nova_hal_shutdown(void);

/**
 * Create HAL context
 */
nova_hal_context_t *nova_hal_context_create(void);

/**
 * Destroy HAL context
 */
void nova_hal_context_destroy(nova_hal_context_t *ctx);

/* ============================================================================
 * Device Management
 * ========================================================================== */

/**
 * Get number of available devices
 */
size_t nova_hal_device_count(nova_device_type_t type);

/**
 * Get device capabilities
 */
const nova_device_caps_t *nova_hal_device_get_caps(size_t device_index, nova_device_type_t type);

/**
 * Create device handle
 */
nova_device_handle_t *nova_hal_device_create(size_t device_index, nova_device_type_t type);

/**
 * Destroy device handle
 */
void nova_hal_device_destroy(nova_device_handle_t *device);

/**
 * Get default CPU device
 */
nova_device_handle_t *nova_hal_device_get_default_cpu(void);

/**
 * Get default GPU device
 */
nova_device_handle_t *nova_hal_device_get_default_gpu(void);

/* ============================================================================
 * Memory Management
 * ========================================================================== */

/**
 * Allocate memory on device
 */
nova_memory_handle_t *nova_hal_memory_allocate(nova_device_handle_t *device,
                                              size_t size,
                                              nova_memory_type_t type);

/**
 * Free memory
 */
void nova_hal_memory_free(nova_memory_handle_t *memory);

/**
 * Copy memory between devices
 */
int nova_hal_memory_copy(nova_memory_handle_t *dst, size_t dst_offset,
                        nova_memory_handle_t *src, size_t src_offset,
                        size_t size);

/**
 * Copy memory from host to device
 */
int nova_hal_memory_copy_to_device(nova_memory_handle_t *dst, size_t dst_offset,
                                  const void *src, size_t size);

/**
 * Copy memory from device to host
 */
int nova_hal_memory_copy_from_device(void *dst,
                                    nova_memory_handle_t *src, size_t src_offset,
                                    size_t size);

/**
 * Map device memory to host address space
 */
void *nova_hal_memory_map(nova_memory_handle_t *memory, size_t offset, size_t size);

/**
 * Unmap device memory
 */
void nova_hal_memory_unmap(nova_memory_handle_t *memory, void *ptr);

/**
 * Synchronize memory operations
 */
int nova_hal_memory_sync(nova_memory_handle_t *memory);

/* ============================================================================
 * Execution Management
 * ========================================================================== */

/**
 * Submit work to device
 */
typedef void (*nova_kernel_func_t)(void *args);

int nova_hal_execute_kernel(nova_device_handle_t *device,
                           nova_kernel_func_t kernel,
                           void *args,
                           size_t args_size);

/**
 * Wait for device operations to complete
 */
int nova_hal_device_sync(nova_device_handle_t *device);

/**
 * Check if device operations are complete
 */
bool nova_hal_device_is_complete(nova_device_handle_t *device);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_HAL_H */
