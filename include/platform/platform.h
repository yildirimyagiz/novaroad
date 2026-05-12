/**
 * @file platform.h
 * @brief Platform detection and abstraction
 */

#ifndef NOVA_PLATFORM_H
#define NOVA_PLATFORM_H

#include "../config/config.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Platform types */
typedef enum {
  NOVA_PLATFORM_TYPE_UNKNOWN,
  NOVA_PLATFORM_TYPE_LINUX,
  NOVA_PLATFORM_TYPE_MACOS,
  NOVA_PLATFORM_TYPE_IOS,
  NOVA_PLATFORM_TYPE_WINDOWS,
  NOVA_PLATFORM_TYPE_ANDROID,
  NOVA_PLATFORM_TYPE_WASM,
  NOVA_PLATFORM_TYPE_FREEBSD,
} nova_platform_type_t;

/* Architecture types */
#ifndef NOVA_ARCH_TYPE_DEFINED
#define NOVA_ARCH_TYPE_DEFINED
typedef enum {
  NOVA_ARCH_TYPE_UNKNOWN,
  NOVA_ARCH_TYPE_X86_64,
  NOVA_ARCH_TYPE_AARCH64,
  NOVA_ARCH_TYPE_RISCV64,
  NOVA_ARCH_TYPE_WASM32,
  NOVA_ARCH_TYPE_WASM64,
} nova_arch_type_t;
#endif

/* Platform categories */
typedef enum {
  NOVA_PLATFORM_CATEGORY_UNKNOWN,
  NOVA_PLATFORM_CATEGORY_DESKTOP,
  NOVA_PLATFORM_CATEGORY_MOBILE,
  NOVA_PLATFORM_CATEGORY_WEB,
  NOVA_PLATFORM_CATEGORY_EMBEDDED,
} nova_platform_category_t;

/* Platform information structure */
typedef struct {
  nova_platform_type_t type;
  nova_arch_type_t arch;
  nova_platform_category_t category;
  const char *name;
  const char *version;
  const char *arch_name;
  bool is_little_endian;
  bool has_simd;
  bool has_gpu;
  size_t page_size;
  size_t cache_line_size;
} nova_platform_info_t;

/**
 * Initialize platform-specific code
 */
int nova_platform_init(void);

/**
 * Get platform information
 */
const nova_platform_info_t *nova_platform_get_info(void);

/**
 * Platform name
 */
const char *nova_platform_name(void);

/**
 * Architecture name
 */
const char *nova_platform_arch(void);

/**
 * Check if platform is little endian
 */
bool nova_platform_is_little_endian(void);

/**
 * Check if platform has SIMD support
 */
bool nova_platform_has_simd(void);

/**
 * Check if platform has GPU support
 */
bool nova_platform_has_gpu(void);

/**
 * Get system page size
 */
size_t nova_platform_page_size(void);

/**
 * Get cache line size
 */
size_t nova_platform_cache_line_size(void);

/**
 * Get number of CPU cores
 */
size_t nova_platform_cpu_count(void);

/**
 * Get total system memory in bytes
 */
size_t nova_platform_total_memory(void);

/**
 * Get available system memory in bytes
 */
size_t nova_platform_available_memory(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_PLATFORM_H */
