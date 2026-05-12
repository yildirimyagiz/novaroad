#ifndef NOVA_DISPATCHER_H
#define NOVA_DISPATCHER_H

#include "nova_context.h"
#include "runtime/nova_execution_fabric.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA DISPATCHER - The Router
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Responsible for routing scheduled tasks to the correct execution backend.
 */

typedef struct {
  void (*kernel)(void *);
  void *args;
  BackendType target_backend;
} DispatchTask;

void nova_dispatcher_init(NovaContext *ctx);
void nova_dispatcher_shutdown(NovaContext *ctx);

// Routing logic
bool nova_dispatcher_route(NovaContext *ctx, DispatchTask task);

#endif // NOVA_DISPATCHER_H
