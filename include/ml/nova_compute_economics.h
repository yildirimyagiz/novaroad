#ifndef NOVA_COMPUTE_ECONOMICS_H
#define NOVA_COMPUTE_ECONOMICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ECONOMIC COMPUTE LAYER
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
    double cpu_price_per_ms;
    double gpu_price_per_ms;
    double memory_price_per_mb;
    double network_price_per_kb;
    double army_price_per_op;
} ResourcePrices;

typedef struct {
    uint64_t cpu_time_ms;
    uint64_t gpu_time_ms;
    size_t memory_used_mb;
    uint64_t network_io_kb;
    double total_cost;
} ComputeBill;

typedef struct NovaComputeEconomics {
    ResourcePrices prices;
    ComputeBill current_session;
    double budget_limit;
} NovaComputeEconomics;

NovaComputeEconomics *nova_economics_init(double budget);
void nova_economics_shutdown(NovaComputeEconomics *e);

// Pricing API
double nova_economics_estimate_cost(NovaComputeEconomics *e, uint64_t cpu, uint64_t gpu);
void nova_economics_track_usage(NovaComputeEconomics *e, uint64_t cpu, uint64_t gpu, size_t mem);

// Optimization Policies
bool nova_economics_can_afford(NovaComputeEconomics *e, double extra_cost);
void nova_economics_report(NovaComputeEconomics *e);

#endif // NOVA_COMPUTE_ECONOMICS_H
