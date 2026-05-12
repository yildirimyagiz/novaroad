#include "../../include/nova_context.h"
#include "../../include/nova_adaptive_optimizer.h"
#include "../../include/nova_cognitive_scheduler.h"
#include "../../include/nova_compute_economics.h"
#include "../../include/nova_execution_fabric.h"
#include "../../include/nova_fault_intelligence.h"
#include "../../include/nova_invariant_engine.h"
#include "../../include/nova_learning_runtime.h"
#include "../../include/nova_profiler_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaContext *nova_context_create(ContextConfig config) {
  NovaContext *ctx = calloc(1, sizeof(NovaContext));
  ctx->config = config;

  printf("🔅 Initializing Nova Context (Kernel State)...\n");

  // Initialize all layers with context awareness
  ctx->fabric = nova_fabric_init();
  ctx->scheduler = nova_scheduler_init();
  ctx->profiler = nova_profiler_init();
  ctx->optimizer = nova_adaptive_init(ctx->profiler);
  ctx->invariant_engine = nova_invariant_init(64 * 1024); // 64KB reserve
  ctx->knowledge_base = nova_learning_init(".nova_kb.db");
  ctx->economics = nova_economics_init(100.0); // 100 quota
  ctx->fault_monitor = nova_fault_init();

  // Setup global memory pool
  ctx->pool_capacity = config.default_pool_size > 0
                           ? config.default_pool_size
                           : (256 * 1024 * 1024); // 256MB default
  ctx->global_memory_pool = malloc(ctx->pool_capacity);

  printf("✅ Nova Context Active. System Logic Clock: 0\n");
  return ctx;
}

void nova_context_destroy(NovaContext *ctx) {
  if (!ctx)
    return;

  printf("🔌 Shutting down Nova Context...\n");

  nova_fabric_shutdown(ctx->fabric);
  nova_scheduler_shutdown(ctx->scheduler);
  nova_profiler_shutdown(ctx->profiler);
  nova_adaptive_shutdown(ctx->optimizer);
  nova_invariant_shutdown(ctx->invariant_engine);
  nova_learning_shutdown(ctx->knowledge_base);
  nova_economics_shutdown(ctx->economics);
  nova_fault_shutdown(ctx->fault_monitor);

  if (ctx->pe_cache.data)
    free(ctx->pe_cache.data);
  free(ctx->global_memory_pool);
  free(ctx);
}

void nova_context_yield(NovaContext *ctx) { ctx->logical_clock++; }

void nova_context_sync(NovaContext *ctx) {
  // Invariant check on sync points
  if (!nova_invariant_check_all(ctx->invariant_engine)) {
    printf("🚨 Context Sync Failed: Invariant Violation\n");
  }
}

NovaContext *nova_context_create_default(void) {
  ContextConfig cfg = {.strict_determinism = false,
                       .enable_profiling = true,
                       .adaptive_optimization = true,
                       .default_pool_size = 256 * 1024 * 1024};
  return nova_context_create(cfg);
}
