#include "../../include/nova_compute_economics.h"
#include <stdio.h>
#include <stdlib.h>

NovaComputeEconomics *nova_economics_init(double budget) {
  NovaComputeEconomics *e = calloc(1, sizeof(NovaComputeEconomics));

  // Set standard prices (simulated currency)
  e->prices = (ResourcePrices){.cpu_price_per_ms = 0.0001,
                               .gpu_price_per_ms = 0.001,
                               .memory_price_per_mb = 0.00005,
                               .network_price_per_kb = 0.0002};

  e->budget_limit = budget;

  printf("💰 Nova Economic Compute Layer Active (Budget: %.4f)\n", budget);
  return e;
}

void nova_economics_shutdown(NovaComputeEconomics *e) {
  if (!e)
    return;
  nova_economics_report(e);
  free(e);
}

double nova_economics_estimate_cost(NovaComputeEconomics *e, uint64_t cpu,
                                      uint64_t gpu) {
  return (cpu * e->prices.cpu_price_per_ms) +
         (gpu * e->prices.gpu_price_per_ms);
}

void nova_economics_track_usage(NovaComputeEconomics *e, uint64_t cpu,
                                  uint64_t gpu, size_t mem) {
  e->current_session.cpu_time_ms += cpu;
  e->current_session.gpu_time_ms += gpu;
  e->current_session.memory_used_mb = mem > e->current_session.memory_used_mb
                                          ? mem
                                          : e->current_session.memory_used_mb;

  double incremental_cost =
      (cpu * e->prices.cpu_price_per_ms) + (gpu * e->prices.gpu_price_per_ms);
  e->current_session.total_cost += incremental_cost;

  if (e->current_session.total_cost > e->budget_limit * 0.9) {
    printf("⚠️ Economic Alert: Budget usage at %.1f%%\n",
           (e->current_session.total_cost / e->budget_limit) * 100);
  }
}

bool nova_economics_can_afford(NovaComputeEconomics *e, double extra_cost) {
  return (e->current_session.total_cost + extra_cost) <= e->budget_limit;
}

void nova_economics_report(NovaComputeEconomics *e) {
  printf("\n🧾 NOVA COMPUTE BILLING REPORT\n");
  printf("---------------------------------------------------------------------"
         "-----------\n");
  printf("CPU Time:     %llu ms\n",
         (unsigned long long)e->current_session.cpu_time_ms);
  printf("GPU Time:     %llu ms\n",
         (unsigned long long)e->current_session.gpu_time_ms);
  printf("Peak Memory:  %zu MB\n", e->current_session.memory_used_mb);
  printf("---------------------------------------------------------------------"
         "-----------\n");
  printf("TOTAL COST:   %.6f units\n", e->current_session.total_cost);
  printf("BUDGET LEFT:  %.6f units\n",
         e->budget_limit - e->current_session.total_cost);
  printf("---------------------------------------------------------------------"
         "-----------\n\n");
}
