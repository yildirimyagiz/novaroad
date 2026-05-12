#include "nova_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void simulate_monitoring() {
  NovaMonitorContext *ctx =
      nova_monitor_create(0.02f); // 2% significance threshold
  NovaSystemMetrics metrics = {0};

  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  Nova Sovereignty Monitoring: Server Farm Simulation\n");
  printf("  Threshold: 2%% change required to trigger update\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // Simulate 100 seconds of monitoring
  for (int t = 0; t < 100; t++) {
    // Simulate slightly fluctuating metrics
    metrics.cpu_usage =
        0.20f + (rand() % 100) / 10000.0f; // Minimal fluctuation (0.1%)
    metrics.memory_usage = 0.50f;

    // Every 30 seconds, simulate a significant event
    if (t == 30) {
      printf("  [t=30s] 💥 Spike detected: CPU usage jumped to 85%%\n");
      metrics.cpu_usage = 0.85f;
    }
    if (t == 60) {
      printf("  [t=60s] 🌡️ Thermal event: Temp increased by 15 degrees\n");
      metrics.temperature = 0.65f;
    }

    uint32_t wire_size = 0;
    uint8_t *buffer = nova_monitor_process(ctx, &metrics, &wire_size);

    if (buffer) {
      printf("  [t=%02ds] Packet Sent! 📡  Wire Size: %u bytes\n", t,
             wire_size);
      free(buffer);
    }
  }

  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  Monitoring Summary (100 Samples)\n");
  printf("  Updates Suppressed (Sub-threshold): %u\n", ctx->updates_filtered);
  printf("  Actual Packets Sent:                %u\n", ctx->packets_sent);
  printf("  Bandwidth Savings:                  %.1f x\n",
         100.0 / ctx->packets_sent);
  printf("════════════════════════════════════════════════════════════\n\n");

  nova_monitor_destroy(ctx);
}

int main() {
  simulate_monitoring();
  return 0;
}
