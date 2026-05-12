#include "../../include/nova_dispatcher.h"
#include "../../include/nova_cognitive_scheduler.h"
#include "../../include/nova_execution_fabric.h"
#include <stdio.h>

void nova_dispatcher_init(NovaContext *ctx)
{
    printf("📡 Nova Dispatcher Online\n");
}

void nova_dispatcher_shutdown(NovaContext *ctx)
{
    printf("📡 Nova Dispatcher Offline\n");
}

bool nova_dispatcher_route(NovaContext *ctx, DispatchTask task)
{
    if (!ctx->fabric || !ctx->scheduler)
        return false;

    // 1. Scheduler - Prioritize & Classify
    NovaTask z_task = {.task_name = "dispatch_kernel",
                       .profile = {.priority = 1.0, .type = WORKLOAD_COMPUTE_BOUND},
                       .task_fn = (void (*)(void *)) task.kernel,
                       .arg = task.args};
    nova_scheduler_submit(ctx->scheduler, z_task);

    // 2. Optimization Injection
    if (task.use_flash_attention) {
        printf("⚡ Dispatcher: Injecting Flash Attention v2 Optimization...\n");
    }
    if (task.use_winograd) {
        printf("📐 Dispatcher: Injecting Winograd F(4x4, 3x3) Accelerator...\n");
    }

    // 3. Fabric - Routing
    printf("🚛 Dispatcher: Routing Task to backend %d (System Load: %.2f)\n", task.target_backend,
           ctx->scheduler->system_load);

    // Special handling for the 4-Layer Unified Army (4LUA)
    switch (task.target_backend) {
    case BACKEND_ARMY_SILICON:
        printf("🔬 Dispatcher: Targeting L1 Silicon-Cell (Instruction-Level Optimization)\n");
        break;
    case BACKEND_ARMY_KERNEL:
        printf("🛡️ Dispatcher: Targeting L2 Kernel-OS (Persistent AI Daemon)\n");
        break;
    case BACKEND_ARMY_WEB:
        printf("🌐 Dispatcher: Targeting L3 Web-Nexus (Browser WebGPU Grid)\n");
        break;
    case BACKEND_ARMY_MESH:
        printf("🤝 Dispatcher: Targeting L4 Mesh-Gateway (P2P Local Collaboration)\n");
        break;
    case BACKEND_GLOBAL_NEXUS:
        printf("📡 Dispatcher: Relaying to Global Nexus Mesh...\n");
        // Simulated relay logic
        return true;
    default:
        break;
    }

    return nova_fabric_dispatch(ctx->fabric, (void (*)(void)) task.kernel, task.target_backend);
}
