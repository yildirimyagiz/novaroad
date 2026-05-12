/**
 * @file hal.c
 * @brief Hardware Abstraction Layer implementation
 */

#include "platform/hal.h"
#include "platform/platform.h"
#include "std/alloc.h"
#include <string.h>
#include <stdlib.h>

/* Internal structures */
struct nova_memory_handle {
    void *ptr;
    size_t size;
    nova_memory_type_t type;
    nova_device_handle_t *device;
};

struct nova_device_handle {
    nova_device_type_t type;
    size_t device_index;
    nova_device_caps_t caps;
};

struct nova_hal_context {
    bool initialized;
    nova_device_handle_t *cpu_device;
};

/* Global context */
static nova_hal_context_t g_hal_context = {0};

/* ============================================================================
 * Internal Functions
 * ========================================================================== */

static nova_device_caps_t create_cpu_caps(void) {
    nova_device_caps_t caps = {0};
    caps.type = NOVA_DEVICE_TYPE_CPU;
    caps.name = "CPU";
    caps.vendor = "Host";
    caps.total_memory = nova_platform_total_memory();
    caps.compute_units = nova_platform_cpu_count();
    caps.supports_fp64 = true;
    caps.supports_fp32 = true;
    caps.supports_fp16 = true;
    caps.supports_int64 = true;
    caps.supports_int32 = true;
    caps.supports_int16 = true;
    caps.supports_int8 = true;
    caps.supports_async = true;
    caps.supports_unified_memory = true;
    return caps;
}

static nova_device_caps_t create_gpu_caps(void) {
    nova_device_caps_t caps = {0};
    caps.type = NOVA_DEVICE_TYPE_GPU;

    if (nova_platform_has_gpu()) {
#if defined(NOVA_PLATFORM_MACOS) || defined(NOVA_PLATFORM_IOS)
        caps.name = "Apple GPU";
        caps.vendor = "Apple";
        caps.total_memory = 1024 * 1024 * 1024; // 1GB placeholder
        caps.compute_units = 8; // Placeholder
        caps.supports_fp64 = false;
        caps.supports_fp32 = true;
        caps.supports_fp16 = true;
        caps.supports_int64 = true;
        caps.supports_int32 = true;
        caps.supports_int16 = true;
        caps.supports_int8 = true;
        caps.supports_async = true;
        caps.supports_unified_memory = true;
#else
        caps.name = "Generic GPU";
        caps.vendor = "Unknown";
        caps.total_memory = 512 * 1024 * 1024; // 512MB placeholder
        caps.compute_units = 4; // Placeholder
        caps.supports_fp64 = false;
        caps.supports_fp32 = true;
        caps.supports_fp16 = true;
        caps.supports_int64 = false;
        caps.supports_int32 = true;
        caps.supports_int16 = true;
        caps.supports_int8 = true;
        caps.supports_async = true;
        caps.supports_unified_memory = false;
#endif
    } else {
        caps.name = "No GPU";
        caps.vendor = "None";
        caps.total_memory = 0;
        caps.compute_units = 0;
        caps.supports_fp64 = false;
        caps.supports_fp32 = false;
        caps.supports_fp16 = false;
        caps.supports_int64 = false;
        caps.supports_int32 = false;
        caps.supports_int16 = false;
        caps.supports_int8 = false;
        caps.supports_async = false;
        caps.supports_unified_memory = false;
    }

    return caps;
}

/* ============================================================================
 * HAL Context Management
 * ========================================================================== */

int nova_hal_init(void) {
    if (g_hal_context.initialized) {
        return 0;
    }

    // Initialize platform first
    if (nova_platform_init() != 0) {
        return -1;
    }

    g_hal_context.cpu_device = nova_alloc(sizeof(nova_device_handle_t));
    if (!g_hal_context.cpu_device) {
        return -1;
    }

    g_hal_context.cpu_device->type = NOVA_DEVICE_TYPE_CPU;
    g_hal_context.cpu_device->device_index = 0;
    g_hal_context.cpu_device->caps = create_cpu_caps();

    g_hal_context.initialized = true;
    return 0;
}

void nova_hal_shutdown(void) {
    if (!g_hal_context.initialized) {
        return;
    }

    if (g_hal_context.cpu_device) {
        nova_free(g_hal_context.cpu_device);
        g_hal_context.cpu_device = NULL;
    }

    g_hal_context.initialized = false;
}

nova_hal_context_t *nova_hal_context_create(void) {
    if (!g_hal_context.initialized) {
        if (nova_hal_init() != 0) {
            return NULL;
        }
    }

    nova_hal_context_t *ctx = nova_alloc(sizeof(nova_hal_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->initialized = true;
    ctx->cpu_device = g_hal_context.cpu_device;

    return ctx;
}

void nova_hal_context_destroy(nova_hal_context_t *ctx) {
    if (ctx) {
        nova_free(ctx);
    }
}

/* ============================================================================
 * Device Management
 * ========================================================================== */

size_t nova_hal_device_count(nova_device_type_t type) {
    switch (type) {
        case NOVA_DEVICE_TYPE_CPU:
            return 1; // Always at least 1 CPU
        case NOVA_DEVICE_TYPE_GPU:
            return nova_platform_has_gpu() ? 1 : 0;
        default:
            return 0;
    }
}

const nova_device_caps_t *nova_hal_device_get_caps(size_t device_index, nova_device_type_t type) {
    if (!g_hal_context.initialized) {
        return NULL;
    }

    switch (type) {
        case NOVA_DEVICE_TYPE_CPU:
            if (device_index == 0) {
                return &g_hal_context.cpu_device->caps;
            }
            break;
        case NOVA_DEVICE_TYPE_GPU:
            if (device_index == 0 && nova_platform_has_gpu()) {
                static nova_device_caps_t gpu_caps = {0};
                if (gpu_caps.type == 0) { // Initialize on first call
                    gpu_caps = create_gpu_caps();
                }
                return &gpu_caps;
            }
            break;
        default:
            break;
    }

    return NULL;
}

nova_device_handle_t *nova_hal_device_create(size_t device_index, nova_device_type_t type) {
    if (!g_hal_context.initialized) {
        return NULL;
    }

    nova_device_handle_t *device = nova_alloc(sizeof(nova_device_handle_t));
    if (!device) {
        return NULL;
    }

    device->type = type;
    device->device_index = device_index;

    switch (type) {
        case NOVA_DEVICE_TYPE_CPU:
            if (device_index == 0) {
                device->caps = create_cpu_caps();
                return device;
            }
            break;
        case NOVA_DEVICE_TYPE_GPU:
            if (device_index == 0 && nova_platform_has_gpu()) {
                device->caps = create_gpu_caps();
                return device;
            }
            break;
        default:
            break;
    }

    nova_free(device);
    return NULL;
}

void nova_hal_device_destroy(nova_device_handle_t *device) {
    if (device) {
        nova_free(device);
    }
}

nova_device_handle_t *nova_hal_device_get_default_cpu(void) {
    if (!g_hal_context.initialized) {
        return NULL;
    }
    return g_hal_context.cpu_device;
}

nova_device_handle_t *nova_hal_device_get_default_gpu(void) {
    if (!nova_platform_has_gpu()) {
        return NULL;
    }

    return nova_hal_device_create(0, NOVA_DEVICE_TYPE_GPU);
}

/* ============================================================================
 * Memory Management
 * ========================================================================== */

nova_memory_handle_t *nova_hal_memory_allocate(nova_device_handle_t *device,
                                              size_t size,
                                              nova_memory_type_t type) {
    if (!device || size == 0) {
        return NULL;
    }

    nova_memory_handle_t *memory = nova_alloc(sizeof(nova_memory_handle_t));
    if (!memory) {
        return NULL;
    }

    memory->size = size;
    memory->type = type;
    memory->device = device;

    // For CPU and unified memory, just use regular allocation
    if (type == NOVA_MEMORY_TYPE_HOST || type == NOVA_MEMORY_TYPE_UNIFIED) {
        memory->ptr = nova_alloc(size);
        if (!memory->ptr) {
            nova_free(memory);
            return NULL;
        }
    } else {
        // Device memory not implemented yet
        nova_free(memory);
        return NULL;
    }

    return memory;
}

void nova_hal_memory_free(nova_memory_handle_t *memory) {
    if (!memory) {
        return;
    }

    if (memory->ptr) {
        nova_free(memory->ptr);
    }

    nova_free(memory);
}

int nova_hal_memory_copy(nova_memory_handle_t *dst, size_t dst_offset,
                        nova_memory_handle_t *src, size_t src_offset,
                        size_t size) {
    if (!dst || !src || !dst->ptr || !src->ptr) {
        return -1;
    }

    if (dst_offset + size > dst->size || src_offset + size > src->size) {
        return -1;
    }

    memcpy((char*)dst->ptr + dst_offset, (char*)src->ptr + src_offset, size);
    return 0;
}

int nova_hal_memory_copy_to_device(nova_memory_handle_t *dst, size_t dst_offset,
                                  const void *src, size_t size) {
    if (!dst || !dst->ptr || !src) {
        return -1;
    }

    if (dst_offset + size > dst->size) {
        return -1;
    }

    memcpy((char*)dst->ptr + dst_offset, src, size);
    return 0;
}

int nova_hal_memory_copy_from_device(void *dst,
                                    nova_memory_handle_t *src, size_t src_offset,
                                    size_t size) {
    if (!dst || !src || !src->ptr) {
        return -1;
    }

    if (src_offset + size > src->size) {
        return -1;
    }

    memcpy(dst, (char*)src->ptr + src_offset, size);
    return 0;
}

void *nova_hal_memory_map(nova_memory_handle_t *memory, size_t offset, size_t size) {
    if (!memory || !memory->ptr) {
        return NULL;
    }

    if (offset + size > memory->size) {
        return NULL;
    }

    return (char*)memory->ptr + offset;
}

void nova_hal_memory_unmap(nova_memory_handle_t *memory, void *ptr) {
    // For CPU memory, no-op since it's always mapped
    (void)memory;
    (void)ptr;
}

int nova_hal_memory_sync(nova_memory_handle_t *memory) {
    // For CPU memory, no-op
    (void)memory;
    return 0;
}

/* ============================================================================
 * Execution Management
 * ========================================================================== */

int nova_hal_execute_kernel(nova_device_handle_t *device,
                           nova_kernel_func_t kernel,
                           void *args,
                           size_t args_size) {
    if (!device || !kernel) {
        return -1;
    }

    // For CPU, just execute directly
    if (device->type == NOVA_DEVICE_TYPE_CPU) {
        kernel(args);
        return 0;
    }

    // GPU execution not implemented yet
    return -1;
}

int nova_hal_device_sync(nova_device_handle_t *device) {
    // For CPU, no-op
    (void)device;
    return 0;
}

bool nova_hal_device_is_complete(nova_device_handle_t *device) {
    // For CPU, always complete
    (void)device;
    return true;
}
