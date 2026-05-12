/**
 * Nova Runtime Sandbox
 * Secure execution environment with resource limits and isolation
 */

#ifndef NOVA_SANDBOX_H
#define NOVA_SANDBOX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX CAPABILITIES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  SANDBOX_CAP_FILE_READ    = 1 << 0,  // Allow file reading
  SANDBOX_CAP_FILE_WRITE   = 1 << 1,  // Allow file writing
  SANDBOX_CAP_NETWORK      = 1 << 2,  // Allow network access
  SANDBOX_CAP_PROCESS      = 1 << 3,  // Allow process creation
  SANDBOX_CAP_SYSTEM       = 1 << 4,  // Allow system calls
  SANDBOX_CAP_MEMORY_EXEC  = 1 << 5,  // Allow executable memory
  SANDBOX_CAP_THREADS      = 1 << 6,  // Allow thread creation
  SANDBOX_CAP_ALL          = 0xFFFF,  // All capabilities
  SANDBOX_CAP_NONE         = 0        // No capabilities (maximum security)
} SandboxCapability;

// ═══════════════════════════════════════════════════════════════════════════
// RESOURCE LIMITS
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // Memory limits
  size_t max_memory_bytes;      // Maximum heap memory (0 = unlimited)
  size_t max_stack_bytes;       // Maximum stack size
  
  // CPU limits
  uint64_t max_cpu_time_ms;     // Maximum CPU time in milliseconds
  uint64_t max_wall_time_ms;    // Maximum wall clock time
  
  // I/O limits
  size_t max_file_size_bytes;   // Maximum file size for read/write
  size_t max_io_bytes;          // Maximum total I/O bytes
  
  // Execution limits
  uint32_t max_instructions;    // Maximum instruction count (0 = unlimited)
  uint32_t max_recursion_depth; // Maximum recursion depth
  uint32_t max_threads;         // Maximum number of threads
} ResourceLimits;

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  uint32_t capabilities;        // Bitmask of SandboxCapability
  ResourceLimits limits;
  
  // File system isolation
  char **allowed_read_paths;    // Whitelist of readable paths
  size_t allowed_read_count;
  char **allowed_write_paths;   // Whitelist of writable paths
  size_t allowed_write_count;
  
  // Network isolation
  char **allowed_hosts;         // Whitelist of network hosts
  size_t allowed_host_count;
  uint16_t *allowed_ports;      // Whitelist of network ports
  size_t allowed_port_count;
  
  // Security options
  bool enable_aslr;             // Address Space Layout Randomization
  bool enable_stack_canary;     // Stack overflow protection
  bool enable_seccomp;          // Syscall filtering (Linux)
  bool read_only_code;          // Make code section read-only
} SandboxConfig;

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  SandboxConfig config;
  
  // Runtime statistics
  size_t current_memory_bytes;
  uint64_t elapsed_cpu_time_ms;
  uint64_t elapsed_wall_time_ms;
  size_t total_io_bytes;
  uint32_t instruction_count;
  uint32_t recursion_depth;
  
  // Violation tracking
  bool violated;
  char violation_reason[256];
  
  // Platform-specific data
  void *platform_data;
} SandboxContext;

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX API
// ═══════════════════════════════════════════════════════════════════════════

// Lifecycle
SandboxContext *sandbox_create(const SandboxConfig *config);
void sandbox_destroy(SandboxContext *ctx);

// Configuration
SandboxConfig sandbox_config_default(void);
SandboxConfig sandbox_config_strict(void);  // Maximum security
SandboxConfig sandbox_config_permissive(void);  // Minimal restrictions
void sandbox_config_add_capability(SandboxConfig *config, SandboxCapability cap);
void sandbox_config_remove_capability(SandboxConfig *config, SandboxCapability cap);

// Path management
void sandbox_allow_read_path(SandboxConfig *config, const char *path);
void sandbox_allow_write_path(SandboxConfig *config, const char *path);
bool sandbox_is_path_readable(const SandboxContext *ctx, const char *path);
bool sandbox_is_path_writable(const SandboxContext *ctx, const char *path);

// Execution control
bool sandbox_enter(SandboxContext *ctx);
void sandbox_exit(SandboxContext *ctx);
bool sandbox_is_active(const SandboxContext *ctx);

// Resource tracking
void sandbox_track_memory(SandboxContext *ctx, size_t bytes);
void sandbox_track_io(SandboxContext *ctx, size_t bytes);
void sandbox_track_instructions(SandboxContext *ctx, uint32_t count);
bool sandbox_check_limits(SandboxContext *ctx);

// Violation handling
bool sandbox_has_violation(const SandboxContext *ctx);
const char *sandbox_get_violation_reason(const SandboxContext *ctx);
void sandbox_set_violation(SandboxContext *ctx, const char *reason);

// Statistics
void sandbox_get_stats(const SandboxContext *ctx, ResourceLimits *out_stats);
void sandbox_reset_stats(SandboxContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX EXECUTION
// ═══════════════════════════════════════════════════════════════════════════

typedef void (*SandboxedFunction)(void *user_data);

// Execute function in sandbox
bool sandbox_execute(SandboxContext *ctx, SandboxedFunction func, void *user_data);

// Execute with timeout
bool sandbox_execute_timeout(SandboxContext *ctx, SandboxedFunction func, 
                              void *user_data, uint64_t timeout_ms);

#endif // NOVA_SANDBOX_H
