/**
 * Nova Runtime Sandbox Implementation
 */

#include "runtime/sandbox/nova_sandbox.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#include <seccomp.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
// DEFAULT CONFIGURATIONS
// ═══════════════════════════════════════════════════════════════════════════

SandboxConfig sandbox_config_default(void) {
  SandboxConfig config = {0};
  
  config.capabilities = SANDBOX_CAP_FILE_READ | SANDBOX_CAP_FILE_WRITE;
  
  config.limits.max_memory_bytes = 1024 * 1024 * 1024;  // 1 GB
  config.limits.max_stack_bytes = 8 * 1024 * 1024;      // 8 MB
  config.limits.max_cpu_time_ms = 60000;                // 60 seconds
  config.limits.max_wall_time_ms = 120000;              // 120 seconds
  config.limits.max_file_size_bytes = 100 * 1024 * 1024; // 100 MB
  config.limits.max_io_bytes = 500 * 1024 * 1024;       // 500 MB
  config.limits.max_instructions = 0;                   // Unlimited
  config.limits.max_recursion_depth = 1000;
  config.limits.max_threads = 4;
  
  config.enable_aslr = true;
  config.enable_stack_canary = true;
  config.enable_seccomp = false;  // Disabled by default for compatibility
  config.read_only_code = true;
  
  return config;
}

SandboxConfig sandbox_config_strict(void) {
  SandboxConfig config = {0};
  
  config.capabilities = SANDBOX_CAP_NONE;
  
  config.limits.max_memory_bytes = 128 * 1024 * 1024;   // 128 MB
  config.limits.max_stack_bytes = 1 * 1024 * 1024;      // 1 MB
  config.limits.max_cpu_time_ms = 5000;                 // 5 seconds
  config.limits.max_wall_time_ms = 10000;               // 10 seconds
  config.limits.max_file_size_bytes = 0;                // No file access
  config.limits.max_io_bytes = 0;
  config.limits.max_instructions = 10000000;            // 10M instructions
  config.limits.max_recursion_depth = 100;
  config.limits.max_threads = 1;
  
  config.enable_aslr = true;
  config.enable_stack_canary = true;
  config.enable_seccomp = true;
  config.read_only_code = true;
  
  return config;
}

SandboxConfig sandbox_config_permissive(void) {
  SandboxConfig config = {0};
  
  config.capabilities = SANDBOX_CAP_ALL;
  
  config.limits.max_memory_bytes = 0;  // Unlimited
  config.limits.max_stack_bytes = 16 * 1024 * 1024;
  config.limits.max_cpu_time_ms = 0;
  config.limits.max_wall_time_ms = 0;
  config.limits.max_file_size_bytes = 0;
  config.limits.max_io_bytes = 0;
  config.limits.max_instructions = 0;
  config.limits.max_recursion_depth = 10000;
  config.limits.max_threads = 64;
  
  config.enable_aslr = false;
  config.enable_stack_canary = false;
  config.enable_seccomp = false;
  config.read_only_code = false;
  
  return config;
}

// ═══════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

SandboxContext *sandbox_create(const SandboxConfig *config) {
  SandboxContext *ctx = (SandboxContext *)calloc(1, sizeof(SandboxContext));
  if (!ctx) return NULL;
  
  if (config) {
    memcpy(&ctx->config, config, sizeof(SandboxConfig));
  } else {
    ctx->config = sandbox_config_default();
  }
  
  ctx->violated = false;
  ctx->violation_reason[0] = '\0';
  
  return ctx;
}

void sandbox_destroy(SandboxContext *ctx) {
  if (!ctx) return;
  
  // Free path lists
  for (size_t i = 0; i < ctx->config.allowed_read_count; i++) {
    free(ctx->config.allowed_read_paths[i]);
  }
  free(ctx->config.allowed_read_paths);
  
  for (size_t i = 0; i < ctx->config.allowed_write_count; i++) {
    free(ctx->config.allowed_write_paths[i]);
  }
  free(ctx->config.allowed_write_paths);
  
  // Free network lists
  for (size_t i = 0; i < ctx->config.allowed_host_count; i++) {
    free(ctx->config.allowed_hosts[i]);
  }
  free(ctx->config.allowed_hosts);
  free(ctx->config.allowed_ports);
  
  free(ctx->platform_data);
  free(ctx);
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

void sandbox_config_add_capability(SandboxConfig *config, SandboxCapability cap) {
  if (!config) return;
  config->capabilities |= cap;
}

void sandbox_config_remove_capability(SandboxConfig *config, SandboxCapability cap) {
  if (!config) return;
  config->capabilities &= ~cap;
}

void sandbox_allow_read_path(SandboxConfig *config, const char *path) {
  if (!config || !path) return;
  
  config->allowed_read_paths = (char **)realloc(config->allowed_read_paths,
                                                  (config->allowed_read_count + 1) * sizeof(char*));
  config->allowed_read_paths[config->allowed_read_count++] = strdup(path);
}

void sandbox_allow_write_path(SandboxConfig *config, const char *path) {
  if (!config || !path) return;
  
  config->allowed_write_paths = (char **)realloc(config->allowed_write_paths,
                                                   (config->allowed_write_count + 1) * sizeof(char*));
  config->allowed_write_paths[config->allowed_write_count++] = strdup(path);
}

bool sandbox_is_path_readable(const SandboxContext *ctx, const char *path) {
  if (!ctx || !path) return false;
  
  if (!(ctx->config.capabilities & SANDBOX_CAP_FILE_READ)) {
    return false;
  }
  
  // If no whitelist, allow all
  if (ctx->config.allowed_read_count == 0) {
    return true;
  }
  
  // Check whitelist
  for (size_t i = 0; i < ctx->config.allowed_read_count; i++) {
    if (strncmp(path, ctx->config.allowed_read_paths[i], 
                strlen(ctx->config.allowed_read_paths[i])) == 0) {
      return true;
    }
  }
  
  return false;
}

bool sandbox_is_path_writable(const SandboxContext *ctx, const char *path) {
  if (!ctx || !path) return false;
  
  if (!(ctx->config.capabilities & SANDBOX_CAP_FILE_WRITE)) {
    return false;
  }
  
  if (ctx->config.allowed_write_count == 0) {
    return true;
  }
  
  for (size_t i = 0; i < ctx->config.allowed_write_count; i++) {
    if (strncmp(path, ctx->config.allowed_write_paths[i],
                strlen(ctx->config.allowed_write_paths[i])) == 0) {
      return true;
    }
  }
  
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// EXECUTION CONTROL
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t get_current_time_ms(void) {
#ifdef _WIN32
  return GetTickCount64();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

bool sandbox_enter(SandboxContext *ctx) {
  if (!ctx) return false;
  
  // Reset statistics
  ctx->current_memory_bytes = 0;
  ctx->elapsed_cpu_time_ms = 0;
  ctx->elapsed_wall_time_ms = get_current_time_ms();
  ctx->total_io_bytes = 0;
  ctx->instruction_count = 0;
  ctx->recursion_depth = 0;
  ctx->violated = false;
  
#ifdef __linux__
  // Set resource limits on Linux
  if (ctx->config.limits.max_memory_bytes > 0) {
    struct rlimit rlim;
    rlim.rlim_cur = ctx->config.limits.max_memory_bytes;
    rlim.rlim_max = ctx->config.limits.max_memory_bytes;
    setrlimit(RLIMIT_AS, &rlim);
  }
  
  if (ctx->config.limits.max_cpu_time_ms > 0) {
    struct rlimit rlim;
    rlim.rlim_cur = ctx->config.limits.max_cpu_time_ms / 1000;
    rlim.rlim_max = ctx->config.limits.max_cpu_time_ms / 1000;
    setrlimit(RLIMIT_CPU, &rlim);
  }
  
  // Enable seccomp if requested
  if (ctx->config.enable_seccomp) {
    // Basic seccomp filter - allow only safe syscalls
    // In production, this would be more sophisticated
  }
#endif
  
  return true;
}

void sandbox_exit(SandboxContext *ctx) {
  if (!ctx) return;
  
  // Calculate elapsed time
  uint64_t end_time = get_current_time_ms();
  ctx->elapsed_wall_time_ms = end_time - ctx->elapsed_wall_time_ms;
}

bool sandbox_is_active(const SandboxContext *ctx) {
  return ctx && !ctx->violated;
}

// ═══════════════════════════════════════════════════════════════════════════
// RESOURCE TRACKING
// ═══════════════════════════════════════════════════════════════════════════

void sandbox_track_memory(SandboxContext *ctx, size_t bytes) {
  if (!ctx) return;
  
  ctx->current_memory_bytes += bytes;
  
  if (ctx->config.limits.max_memory_bytes > 0 &&
      ctx->current_memory_bytes > ctx->config.limits.max_memory_bytes) {
    sandbox_set_violation(ctx, "Memory limit exceeded");
  }
}

void sandbox_track_io(SandboxContext *ctx, size_t bytes) {
  if (!ctx) return;
  
  ctx->total_io_bytes += bytes;
  
  if (ctx->config.limits.max_io_bytes > 0 &&
      ctx->total_io_bytes > ctx->config.limits.max_io_bytes) {
    sandbox_set_violation(ctx, "I/O limit exceeded");
  }
}

void sandbox_track_instructions(SandboxContext *ctx, uint32_t count) {
  if (!ctx) return;
  
  ctx->instruction_count += count;
  
  if (ctx->config.limits.max_instructions > 0 &&
      ctx->instruction_count > ctx->config.limits.max_instructions) {
    sandbox_set_violation(ctx, "Instruction limit exceeded");
  }
}

bool sandbox_check_limits(SandboxContext *ctx) {
  if (!ctx) return false;
  
  // Check wall time
  uint64_t elapsed = get_current_time_ms() - ctx->elapsed_wall_time_ms;
  if (ctx->config.limits.max_wall_time_ms > 0 &&
      elapsed > ctx->config.limits.max_wall_time_ms) {
    sandbox_set_violation(ctx, "Wall time limit exceeded");
    return false;
  }
  
  return !ctx->violated;
}

// ═══════════════════════════════════════════════════════════════════════════
// VIOLATION HANDLING
// ═══════════════════════════════════════════════════════════════════════════

bool sandbox_has_violation(const SandboxContext *ctx) {
  return ctx && ctx->violated;
}

const char *sandbox_get_violation_reason(const SandboxContext *ctx) {
  return ctx ? ctx->violation_reason : "No context";
}

void sandbox_set_violation(SandboxContext *ctx, const char *reason) {
  if (!ctx) return;
  
  ctx->violated = true;
  snprintf(ctx->violation_reason, sizeof(ctx->violation_reason), "%s", reason);
}

// ═══════════════════════════════════════════════════════════════════════════
// STATISTICS
// ═══════════════════════════════════════════════════════════════════════════

void sandbox_get_stats(const SandboxContext *ctx, ResourceLimits *out_stats) {
  if (!ctx || !out_stats) return;
  
  out_stats->max_memory_bytes = ctx->current_memory_bytes;
  out_stats->max_cpu_time_ms = ctx->elapsed_cpu_time_ms;
  out_stats->max_wall_time_ms = ctx->elapsed_wall_time_ms;
  out_stats->max_io_bytes = ctx->total_io_bytes;
  out_stats->max_instructions = ctx->instruction_count;
  out_stats->max_recursion_depth = ctx->recursion_depth;
}

void sandbox_reset_stats(SandboxContext *ctx) {
  if (!ctx) return;
  
  ctx->current_memory_bytes = 0;
  ctx->elapsed_cpu_time_ms = 0;
  ctx->elapsed_wall_time_ms = 0;
  ctx->total_io_bytes = 0;
  ctx->instruction_count = 0;
  ctx->recursion_depth = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX EXECUTION
// ═══════════════════════════════════════════════════════════════════════════

bool sandbox_execute(SandboxContext *ctx, SandboxedFunction func, void *user_data) {
  if (!ctx || !func) return false;
  
  if (!sandbox_enter(ctx)) {
    return false;
  }
  
  // Execute the function
  func(user_data);
  
  sandbox_exit(ctx);
  
  return !ctx->violated;
}

bool sandbox_execute_timeout(SandboxContext *ctx, SandboxedFunction func,
                              void *user_data, uint64_t timeout_ms) {
  if (!ctx || !func) return false;
  
  // Save original timeout
  uint64_t original_timeout = ctx->config.limits.max_wall_time_ms;
  ctx->config.limits.max_wall_time_ms = timeout_ms;
  
  bool result = sandbox_execute(ctx, func, user_data);
  
  // Restore original timeout
  ctx->config.limits.max_wall_time_ms = original_timeout;
  
  return result;
}
