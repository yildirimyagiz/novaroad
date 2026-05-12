/**
 * @file capabilities.h
 * @brief Capability-based security system
 * 
 * Nova uses object-capability security model:
 * - Fine-grained permissions
 * - Unforgeable capability tokens
 * - Delegation with attenuation
 * - No ambient authority
 */

#ifndef NOVA_CAPABILITIES_H
#define NOVA_CAPABILITIES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Capability Types
 * ========================================================================== */

typedef uint64_t nova_capability_t;
typedef struct nova_cap_token nova_cap_token_t;

/* Basic capability flags (bitfield) */
#define NOVA_CAP_NONE           0x0000000000000000ULL
#define NOVA_CAP_READ           (1ULL << 0)   /**< Read access */
#define NOVA_CAP_WRITE          (1ULL << 1)   /**< Write access */
#define NOVA_CAP_EXECUTE        (1ULL << 2)   /**< Execute access */
#define NOVA_CAP_DELETE         (1ULL << 3)   /**< Delete access */
#define NOVA_CAP_APPEND         (1ULL << 4)   /**< Append-only write */

/* Resource-specific capabilities */
#define NOVA_CAP_FS             (1ULL << 10)  /**< Filesystem access */
#define NOVA_CAP_NETWORK        (1ULL << 11)  /**< Network access */
#define NOVA_CAP_DEVICE         (1ULL << 12)  /**< Device access */
#define NOVA_CAP_IPC            (1ULL << 13)  /**< IPC access */
#define NOVA_CAP_PROCESS        (1ULL << 14)  /**< Process management */
#define NOVA_CAP_MEMORY         (1ULL << 15)  /**< Memory management */

/* System capabilities (high privilege) */
#define NOVA_CAP_SYS_ADMIN      (1ULL << 20)  /**< System administration */
#define NOVA_CAP_SYS_TIME       (1ULL << 21)  /**< Set system time */
#define NOVA_CAP_SYS_MODULE     (1ULL << 22)  /**< Load kernel modules */
#define NOVA_CAP_SYS_RESOURCE   (1ULL << 23)  /**< Override resource limits */
#define NOVA_CAP_SYS_BOOT       (1ULL << 24)  /**< Reboot system */

/* Special capabilities */
#define NOVA_CAP_DELEGATE       (1ULL << 30)  /**< Can delegate capabilities */
#define NOVA_CAP_ALL            0xFFFFFFFFFFFFFFFFULL /**< All capabilities */

/* ============================================================================
 * Basic Capability Operations
 * ========================================================================== */

/**
 * @brief Check if process has capability
 * @param cap Capability to check
 * @return true if process has capability
 */
bool nova_has_capability(nova_capability_t cap);

/**
 * @brief Check if process has all specified capabilities
 * @param caps Capability bitfield
 * @return true if has all capabilities
 */
bool nova_has_all_capabilities(nova_capability_t caps);

/**
 * @brief Check if process has any of the specified capabilities
 * @param caps Capability bitfield
 * @return true if has at least one capability
 */
bool nova_has_any_capability(nova_capability_t caps);

/**
 * @brief Get current process capabilities
 * @return Current capability set
 */
nova_capability_t nova_get_capabilities(void);

/**
 * @brief Grant capability to current process
 * @param cap Capability to grant
 * @return 0 on success, -1 on failure
 */
int nova_grant_capability(nova_capability_t cap);

/**
 * @brief Revoke capability from current process
 * @param cap Capability to revoke
 * @return 0 on success, -1 on failure
 */
int nova_revoke_capability(nova_capability_t cap);

/**
 * @brief Set exact capability set (replaces current)
 * @param caps New capability set
 * @return 0 on success, -1 on failure
 */
int nova_set_capabilities(nova_capability_t caps);

/**
 * @brief Drop all capabilities (enter restricted mode)
 * @return 0 on success, -1 on failure
 */
int nova_drop_all_capabilities(void);

/* ============================================================================
 * Capability Tokens (Unforgeable)
 * ========================================================================== */

/**
 * @brief Create capability token for resource
 * @param caps Capabilities to grant
 * @param resource_id Resource identifier
 * @param expiry_time Expiration time (0 for no expiration)
 * @return Capability token or NULL on failure
 */
nova_cap_token_t *nova_cap_token_create(nova_capability_t caps,
                                         const char *resource_id,
                                         uint64_t expiry_time);

/**
 * @brief Validate and use capability token
 * @param token Capability token
 * @return Granted capabilities or NOVA_CAP_NONE if invalid
 */
nova_capability_t nova_cap_token_use(nova_cap_token_t *token);

/**
 * @brief Check if token is valid
 * @param token Capability token
 * @return true if valid and not expired
 */
bool nova_cap_token_is_valid(nova_cap_token_t *token);

/**
 * @brief Get capabilities from token
 * @param token Capability token
 * @return Capability set
 */
nova_capability_t nova_cap_token_get_caps(nova_cap_token_t *token);

/**
 * @brief Get resource ID from token
 * @param token Capability token
 * @return Resource identifier
 */
const char *nova_cap_token_get_resource(nova_cap_token_t *token);

/**
 * @brief Revoke capability token
 * @param token Capability token to revoke
 */
void nova_cap_token_revoke(nova_cap_token_t *token);

/**
 * @brief Destroy capability token
 * @param token Token to destroy
 */
void nova_cap_token_destroy(nova_cap_token_t *token);

/* ============================================================================
 * Capability Delegation
 * ========================================================================== */

/**
 * @brief Delegate capabilities to another process
 * @param pid Target process ID
 * @param caps Capabilities to delegate (must be subset of current)
 * @return 0 on success, -1 on failure
 */
int nova_cap_delegate(uint64_t pid, nova_capability_t caps);

/**
 * @brief Delegate with attenuation (reduced permissions)
 * @param token Original token
 * @param new_caps New capability set (must be subset)
 * @return Attenuated token or NULL on failure
 */
nova_cap_token_t *nova_cap_attenuate(nova_cap_token_t *token,
                                     nova_capability_t new_caps);

/**
 * @brief Transfer capability token to another process
 * @param token Token to transfer
 * @param pid Target process ID
 * @return 0 on success, -1 on failure
 */
int nova_cap_transfer(nova_cap_token_t *token, uint64_t pid);

/* ============================================================================
 * Resource-Specific Capability Checks
 * ========================================================================== */

/**
 * @brief Check file access capability
 * @param path File path
 * @param mode Access mode (NOVA_CAP_READ | NOVA_CAP_WRITE, etc.)
 * @return true if access allowed
 */
bool nova_cap_check_file(const char *path, nova_capability_t mode);

/**
 * @brief Check network capability
 * @param host Host/IP address
 * @param port Port number
 * @return true if network access allowed
 */
bool nova_cap_check_network(const char *host, uint16_t port);

/**
 * @brief Check process capability
 * @param pid Process ID
 * @param operation Operation type (signal, kill, etc.)
 * @return true if operation allowed
 */
bool nova_cap_check_process(uint64_t pid, const char *operation);

/**
 * @brief Check device capability
 * @param device_path Device path
 * @param mode Access mode
 * @return true if device access allowed
 */
bool nova_cap_check_device(const char *device_path, nova_capability_t mode);

/* ============================================================================
 * Capability Auditing
 * ========================================================================== */

typedef struct {
    uint64_t timestamp;
    uint64_t pid;
    nova_capability_t caps;
    const char *operation;
    bool granted;
} nova_cap_audit_entry_t;

/**
 * @brief Enable capability auditing
 * @param enabled true to enable auditing
 */
void nova_cap_set_auditing(bool enabled);

/**
 * @brief Get audit log
 * @param count Output: number of entries
 * @return Array of audit entries
 */
const nova_cap_audit_entry_t **nova_cap_get_audit_log(size_t *count);

/**
 * @brief Clear audit log
 */
void nova_cap_clear_audit_log(void);

/* ============================================================================
 * Capability Management
 * ========================================================================== */

/**
 * @brief Save capability state
 * @return Saved capability set
 */
nova_capability_t nova_cap_save(void);

/**
 * @brief Restore capability state
 * @param saved_caps Previously saved capability set
 * @return 0 on success, -1 on failure
 */
int nova_cap_restore(nova_capability_t saved_caps);

/**
 * @brief Get capability name
 * @param cap Capability flag
 * @return Human-readable name
 */
const char *nova_cap_name(nova_capability_t cap);

/**
 * @brief Parse capability from string
 * @param name Capability name (e.g., "read", "write", "network")
 * @return Capability value or NOVA_CAP_NONE if invalid
 */
nova_capability_t nova_cap_from_string(const char *name);

/**
 * @brief Convert capability set to string
 * @param caps Capability set
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Number of bytes written
 */
size_t nova_cap_to_string(nova_capability_t caps, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_CAPABILITIES_H */
