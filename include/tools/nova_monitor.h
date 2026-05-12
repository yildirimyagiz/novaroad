/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_monitor.h - Ultra-Lightweight System Monitoring (Mirror-Based)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_MONITOR_H
#define NOVA_MONITOR_H

#include <stdint.h>

typedef struct {
  float cpu_usage;     // 0.0 - 1.0
  float memory_usage;  // 0.0 - 1.0
  float disk_io;       // MB/s normalized
  float net_io;        // MB/s normalized
  float temperature;   // Degrees Celsius normalized
  float fan_speed;     // RPM normalized
  float power_watt;    // Power draw
  float anomaly_score; // Output from Nova ML internal model
} NovaSystemMetrics;

#define NOVA_METRIC_COUNT (sizeof(NovaSystemMetrics) / sizeof(float))

typedef struct {
  NovaSystemMetrics last_sent;
  float significance_threshold;
  uint32_t updates_filtered;
  uint32_t packets_sent;
} NovaMonitorContext;

NovaMonitorContext *nova_monitor_create(float threshold);
void nova_monitor_destroy(NovaMonitorContext *ctx);

// Returns a wire buffer if significant change detected, otherwise NULL
uint8_t *nova_monitor_process(NovaMonitorContext *ctx,
                                const NovaSystemMetrics *current,
                                uint32_t *out_size);

#endif // NOVA_MONITOR_H
