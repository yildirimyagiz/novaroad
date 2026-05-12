#include "../../include/nova_dispatcher.h"
#include "../../include/nova_cognitive_scheduler.h"
#include "../../include/nova_execution_fabric.h"
#include <stdio.h>

void nova_dispatcher_init(NovaContext *ctx) {
  printf("📡 Nova Dispatcher Online\n");
}

void nova_dispatcher_shutdown(NovaContext *ctx) {
  printf("📡 Nova Dispatcher Offline\n");
}

bool nova_dispatcher_route(NovaContext *ctx, DispatchTask task) {
  if (!ctx->fabric || !ctx->scheduler)
    return false;

  // 1. Scheduler - Prioritize & Classify
  NovaTask z_task = {
      .task_name = "dispatch_kernel",
      .profile = {.priority = 1.0, .type = WORKLOAD_COMPUTE_BOUND},
      .task_fn = (void (*)(void *))task.kernel,
      .arg = task.args};
  nova_scheduler_submit(ctx->scheduler, z_task);

  // 2. Scheduler - Plan
  // In a real system, the scheduler might reorder or delay.
  // Here we simulate it giving the green light.

  printf(
      "🚛 Dispatcher: Task scheduled with load balancing (System Load: %.2f)\n",
      ctx->scheduler->system_load);

  // 3. Fabric - Routing
  printf("🚛 Dispatching task to backend %d (via Fabric)\n",
         task.target_backend);

  return nova_fabric_dispatch(ctx->fabric, (void (*)(void))task.kernel,
                                task.target_backend);
}
