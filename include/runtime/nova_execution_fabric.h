#ifndef NOVA_EXECUTION_FABRIC_H
#define NOVA_EXECUTION_FABRIC_H

#include <stdbool.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA MULTI-BACKEND EXECUTION FABRIC
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
    BACKEND_CPU,
    BACKEND_SIMD_AVX512,
    BACKEND_METAL_GPU,
    BACKEND_CUDA_GPU,

    // 🦅 THE 4-LAYER UNIFIED ARMY (4LUA)
    BACKEND_ARMY_SILICON, // L1: Hardware Instruction Set
    BACKEND_ARMY_KERNEL,  // L2: OS-Level Daemon
    BACKEND_ARMY_WEB,     // L3: Browser WebGPU/WASM
    BACKEND_ARMY_MESH,    // L4: P2P Local Gateway
    BACKEND_REMOTE_NODE,
    BACKEND_GLOBAL_NEXUS
} BackendType;

typedef struct {
    BackendType type;
    bool available;
    uint32_t current_latency_us;
    double cost_per_op;
} NovaBackend;

typedef struct NovaExecutionFabric {
    NovaBackend backends[8];
    int backend_count;

    BackendType primary_backend;
    BackendType fallback_backend;
} NovaExecutionFabric;

NovaExecutionFabric *nova_fabric_init(void);
void nova_fabric_shutdown(NovaExecutionFabric *f);

// Dispatch API
bool nova_fabric_dispatch(NovaExecutionFabric *f, void (*kernel)(void), BackendType preferred);
void nova_fabric_report_latency(NovaExecutionFabric *f, BackendType type, uint32_t latency);

// Failover
void nova_fabric_set_failover(NovaExecutionFabric *f, BackendType primary, BackendType fallback);

#endif // NOVA_EXECUTION_FABRIC_H
