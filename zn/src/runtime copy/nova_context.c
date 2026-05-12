#include "nova_context.h"
#include "formal/nova_proof_cache.h"
#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Create execution context
 */
NovaContext *nova_context_create(ContextConfig config) {
  // Allocate context
  NovaContext *ctx = (NovaContext *)calloc(1, sizeof(NovaContext));
  if (!ctx) {
    fprintf(stderr, "[Nova] ERROR: Failed to allocate context\n");
    return NULL;
  }

  ctx->config = config;

  // Initialize proof cache (if not already done)
  static bool proof_cache_initialized = false;
  if (!proof_cache_initialized) {
    nova_proof_cache_init();
    proof_cache_initialized = true;
  }

  // 🦅 Sovereign Backend Calibration
  nova_autocal_run();
  if (config.formal_mode > 0) {
    nova_autocal_report();
  }

  // Hardware detection & layer initialization would go here
  ctx->active_backend_id = 0;
  ctx->logical_clock = 0;

  if (config.default_pool_size > 0) {
    ctx->global_memory_pool = malloc(config.default_pool_size);
    ctx->pool_capacity = config.default_pool_size;
    ctx->pool_usage = 0;
  }

  printf("[Nova] Context created with formal_mode=%d\n",
         config.formal_mode);

  return ctx;
}

/**
 * Create context with default config (for FFI / Python callers)
 */
NovaContext *nova_context_create_default(void) {
  ContextConfig config;
  memset(&config, 0, sizeof(config));
  config.formal_mode = NOVA_FORMAL_FAST;
  config.default_pool_size = 0;
  return nova_context_create(config);
}

/**
 * Destroy context and cleanup resources
 */
void nova_context_destroy(NovaContext *ctx) {
  if (!ctx) {
    return;
  }

  if (ctx->global_memory_pool) {
    free(ctx->global_memory_pool);
  }

  // Cleanup other layers (fabric, scheduler, etc.)
  // ...

  free(ctx);
  printf("[Nova] Context destroyed\n");
}

void nova_context_sync(NovaContext *ctx) {
  (void)ctx;
  // Sync logic
}

void nova_context_yield(NovaContext *ctx) {
  (void)ctx;
  // Yield logic
}
