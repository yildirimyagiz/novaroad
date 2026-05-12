/**
 * @file sandbox.h
 * @brief Process sandboxing and isolation
 */

#ifndef NOVA_SANDBOX_H
#define NOVA_SANDBOX_H

#include "capabilities.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Sandbox Types
 * ========================================================================== */

typedef struct nova_sandbox nova_sandbox_t;

typedef enum {
    NOVA_SANDBOX_STRICT,        /**< Maximum isolation */
    NOVA_SANDBOX_STANDARD,      /**< Balanced security */
    NOVA_SANDBOX_PERMISSIVE,    /**< Minimal restrictions */
} nova_sandbox_level_t;

/* ============================================================================
 * Sandbox Configuration
 * ========================================================================== */

typedef struct {
    /* Security level */
    nova_sandbox_level_t level;
    nova_capability_t allowed_caps;
    
    /* Resource limits */
    size_t max_memory;              /**< Max memory in bytes */
    uint64_t max_cpu_time_ms;       /**< Max CPU time in milliseconds */
    uint64_t max_wall_time_ms;      /**< Max wall-clock time */
    size_t max_file_size;           /**< Max file size in bytes */
    size_t max_open_files;          /**< Max open file descriptors */
    size_t max_threads;             /**< Max threads */
    
    /* Network restrictions */
    bool allow_network;
    const char **allowed_hosts;     /**< Whitelist of allowed hosts */
    size_t num_allowed_hosts;
    uint16_t *allowed_ports;        /**< Whitelist of allowed ports */
    size_t num_allowed_ports;
    
    /* Filesystem restrictions */
    bool readonly_filesystem;
    const char **allowed_paths;     /**< Whitelist of accessible paths */
    size_t num_allowed_paths;
    const char *chroot_dir;         /**< Chroot directory (NULL for none) */
    
    /* Process restrictions */
    bool allow_fork;
    bool allow_exec;
    const char **allowed_syscalls;  /**< Whitelist of syscalls */
    size_t num_allowed_syscalls;
    
    /* Monitoring */
    bool enable_audit_log;
    const char *audit_log_path;
} nova_sandbox_config_t;

/**
 * Get default sandbox configuration
 * @param level Security level
 * @return Default configuration
 */
nova_sandbox_config_t nova_sandbox_config_default(nova_sandbox_level_t level);

/* ============================================================================
 * Sandbox Creation & Management
 * ========================================================================== */

/**
 * Create sandbox with configuration
 * @param config Sandbox configuration
 * @return Created sandbox or NULL on error
 */
nova_sandbox_t *nova_sandbox_create(const nova_sandbox_config_t *config);

/**
 * Enter sandbox (apply restrictions to current process)
 * @param sandbox The sandbox
 * @return 0 on success, -1 on failure
 */
int nova_sandbox_enter(nova_sandbox_t *sandbox);

/**
 * Exit sandbox (restore original permissions)
 * @param sandbox The sandbox
 */
void nova_sandbox_exit(nova_sandbox_t *sandbox);

/**
 * Check if process is sandboxed
 * @return true if in sandbox
 */
bool nova_sandbox_is_active(void);

/**
 * Get current sandbox
 * @return Current sandbox or NULL if not sandboxed
 */
nova_sandbox_t *nova_sandbox_current(void);

/**
 * Destroy sandbox
 * @param sandbox Sandbox to destroy
 */
void nova_sandbox_destroy(nova_sandbox_t *sandbox);

/* ============================================================================
 * Sandboxed Process Execution
 * ========================================================================== */

/**
 * Execute command in sandbox
 * @param sandbox The sandbox
 * @param command Command to execute
 * @param argv Arguments (NULL-terminated)
 * @param envp Environment (NULL-terminated, NULL for inherit)
 * @return Exit code or negative on error
 */
int nova_sandbox_exec(nova_sandbox_t *sandbox, const char *command,
                     char *const argv[], char *const envp[]);

/**
 * Spawn sandboxed process
 * @param sandbox The sandbox
 * @param command Command to execute
 * @param argv Arguments
 * @param envp Environment
 * @return Process ID or negative on error
 */
int64_t nova_sandbox_spawn(nova_sandbox_t *sandbox, const char *command,
                          char *const argv[], char *const envp[]);

/**
 * Kill sandboxed process
 * @param pid Process ID
 * @param signal Signal to send
 * @return 0 on success, -1 on failure
 */
int nova_sandbox_kill(int64_t pid, int signal);

/* ============================================================================
 * Resource Monitoring
 * ========================================================================== */

typedef struct {
    size_t memory_used;
    uint64_t cpu_time_ms;
    uint64_t wall_time_ms;
    size_t open_files;
    size_t threads;
    size_t syscalls_made;
    size_t violations;
} nova_sandbox_stats_t;

/**
 * Get sandbox statistics
 * @param sandbox The sandbox
 * @param stats Output: statistics
 */
void nova_sandbox_get_stats(nova_sandbox_t *sandbox, nova_sandbox_stats_t *stats);

/**
 * Reset sandbox statistics
 * @param sandbox The sandbox
 */
void nova_sandbox_reset_stats(nova_sandbox_t *sandbox);

/* ============================================================================
 * Violation Handling
 * ========================================================================== */

typedef enum {
    NOVA_VIOLATION_CAPABILITY,
    NOVA_VIOLATION_MEMORY_LIMIT,
    NOVA_VIOLATION_CPU_LIMIT,
    NOVA_VIOLATION_TIME_LIMIT,
    NOVA_VIOLATION_FILE_ACCESS,
    NOVA_VIOLATION_NETWORK_ACCESS,
    NOVA_VIOLATION_SYSCALL,
} nova_violation_type_t;

typedef struct {
    nova_violation_type_t type;
    uint64_t timestamp;
    const char *details;
} nova_violation_t;

typedef void (*nova_violation_callback_t)(const nova_violation_t *violation, void *userdata);

/**
 * Set violation callback
 * @param sandbox The sandbox
 * @param callback Callback function
 * @param userdata User data
 */
void nova_sandbox_set_violation_callback(nova_sandbox_t *sandbox,
                                        nova_violation_callback_t callback,
                                        void *userdata);

/**
 * Get violation log
 * @param sandbox The sandbox
 * @param count Output: number of violations
 * @return Array of violations
 */
const nova_violation_t **nova_sandbox_get_violations(nova_sandbox_t *sandbox,
                                                    size_t *count);

/* ============================================================================
 * Temporary Privilege Elevation
 * ========================================================================== */

/**
 * Temporarily grant capability (requires NOVA_CAP_DELEGATE)
 * @param sandbox The sandbox
 * @param cap Capability to grant
 * @param duration_ms Duration in milliseconds
 * @return 0 on success, -1 on failure
 */
int nova_sandbox_grant_temp(nova_sandbox_t *sandbox, nova_capability_t cap,
                           uint64_t duration_ms);

/**
 * Immediately revoke temporary capability
 * @param sandbox The sandbox
 * @param cap Capability to revoke
 * @return 0 on success, -1 on failure
 */
int nova_sandbox_revoke_temp(nova_sandbox_t *sandbox, nova_capability_t cap);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_SANDBOX_H */
