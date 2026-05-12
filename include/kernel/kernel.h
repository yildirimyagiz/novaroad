/**
 * @file kernel.h
 * @brief Nova kernel core interface
 */

#ifndef NOVA_KERNEL_H
#define NOVA_KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Kernel Boot & Initialization
 * ========================================================================== */

typedef struct {
    const char *cmdline;        /**< Kernel command line */
    size_t memory_size;         /**< Total physical memory */
    void *initrd_start;         /**< Initial ramdisk start */
    size_t initrd_size;         /**< Initial ramdisk size */
    const char *bootloader;     /**< Bootloader name */
} nova_boot_info_t;

/**
 * Initialize Nova kernel
 * @param boot_info Boot information from bootloader
 * @return 0 on success, negative on error
 */
int nova_kernel_init(const nova_boot_info_t *boot_info);

/**
 * Kernel main loop (never returns)
 */
void nova_kernel_run(void) __attribute__((noreturn));

/**
 * Shutdown kernel gracefully
 * @param reboot true to reboot, false to halt
 */
void nova_kernel_shutdown(bool reboot) __attribute__((noreturn));

/**
 * Kernel panic (unrecoverable error)
 * @param message Panic message
 */
void nova_kernel_panic(const char *message) __attribute__((noreturn));

/* ============================================================================
 * Kernel Information
 * ========================================================================== */

/**
 * Get kernel version string
 * @return Version string (e.g., "Nova 0.1.0")
 */
const char *nova_kernel_version(void);

/**
 * Get kernel build date/time
 * @return Build timestamp
 */
const char *nova_kernel_build_time(void);

/**
 * Get kernel uptime in milliseconds
 * @return Uptime in ms
 */
uint64_t nova_kernel_uptime_ms(void);

/**
 * Get system time (Unix timestamp)
 * @return Seconds since epoch
 */
uint64_t nova_kernel_time(void);

/* ============================================================================
 * Kernel Modules
 * ========================================================================== */

#ifndef NOVA_MODULE_TYPE_DEFINED
#define NOVA_MODULE_TYPE_DEFINED
typedef struct nova_module nova_module_t;
#endif

typedef struct {
    const char *name;
    const char *version;
    int (*init)(void);
    void (*exit)(void);
} nova_module_info_t;

/**
 * Load kernel module
 * @param path Module file path
 * @return Module handle or NULL on error
 */
nova_module_t *nova_module_load(const char *path);

/**
 * Unload kernel module
 * @param module Module handle
 * @return 0 on success, -1 on error
 */
int nova_module_unload(nova_module_t *module);

/**
 * Get module symbol
 * @param module Module handle
 * @param symbol Symbol name
 * @return Symbol address or NULL if not found
 */
void *nova_module_get_symbol(nova_module_t *module, const char *symbol);

/* ============================================================================
 * Kernel Statistics
 * ========================================================================== */

typedef struct {
    uint64_t interrupts;
    uint64_t context_switches;
    uint64_t syscalls;
    uint64_t page_faults;
    size_t processes;
    size_t threads;
    size_t memory_used;
    size_t memory_free;
} nova_kernel_stats_t;

/**
 * Get kernel statistics
 * @param stats Output statistics
 */
void nova_kernel_get_stats(nova_kernel_stats_t *stats);

/**
 * Reset kernel statistics
 */
void nova_kernel_reset_stats(void);

/* ============================================================================
 * Kernel Logging
 * ========================================================================== */

typedef enum {
    NOVA_LOG_DEBUG,
    NOVA_LOG_INFO,
    NOVA_LOG_WARN,
    NOVA_LOG_ERROR,
    NOVA_LOG_FATAL,
} nova_log_level_t;

/**
 * Kernel log message
 * @param level Log level
 * @param format Printf-style format
 */
void nova_klog(nova_log_level_t level, const char *format, ...);

/**
 * Set kernel log level
 * @param level Minimum level to log
 */
void nova_klog_set_level(nova_log_level_t level);

/* Convenience macros */
#define KDEBUG(fmt, ...) nova_klog(NOVA_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define KINFO(fmt, ...)  nova_klog(NOVA_LOG_INFO, fmt, ##__VA_ARGS__)
#define KWARN(fmt, ...)  nova_klog(NOVA_LOG_WARN, fmt, ##__VA_ARGS__)
#define KERROR(fmt, ...) nova_klog(NOVA_LOG_ERROR, fmt, ##__VA_ARGS__)
#define KPANIC(fmt, ...) nova_kernel_panic(fmt)

#ifdef __cplusplus
}
#endif

#endif /* NOVA_KERNEL_H */
