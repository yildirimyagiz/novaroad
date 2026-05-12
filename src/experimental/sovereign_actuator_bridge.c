/**
 * ═══════════════════════════════════════════════════════════════════════════
 * SOVEREIGN ACTUATOR BRIDGE - Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Hardware control bridge optimized for:
 * - Minimal latency (syscall optimization)
 * - Safe fallback (simulation mode)
 * - Cross-platform stability
 */

#include "../include/sovereign_actuator_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
// HARDWARE CONTROLS
// ═══════════════════════════════════════════════════════════════════════════

bool hw_set_fan_pwm(float pwm_0_1) {
  // Clamp inputs for safety
  if (pwm_0_1 < 0.0f)
    pwm_0_1 = 0.0f;
  if (pwm_0_1 > 1.0f)
    pwm_0_1 = 1.0f;

  // Convert to PWM byte (0-255) for standard controllers
  int pwm_val = (int)(pwm_0_1 * 255.0f);

#if defined(__linux__)
  // Try standard hwmon interface (Requires root/permissions)
  // Optimistic attempt to write to sysfs
  static const char *pwm_path = "/sys/class/hwmon/hwmon0/pwm1";

  FILE *f = fopen(pwm_path, "w");
  if (f) {
    fprintf(f, "%d\n", pwm_val);
    fclose(f);
    return true;
  }
#endif

  // Simulation / Default fallback
  // In a high-perf loop, we might want to suppress this print or use a verbose
  // flag For now, print once per change could be better, but simple print is
  // safe.
  printf("⚙️ [HW] Set Fan PWM: %.2f (%d/255)\n", pwm_0_1, pwm_val);
  return true;
}

bool hw_set_throttle(float ratio_0_1) {
  // CPU frequency scaling control or power limit
  if (ratio_0_1 < 0.0f)
    ratio_0_1 = 0.0f;
  if (ratio_0_1 > 1.0f)
    ratio_0_1 = 1.0f;

  printf("⚡ [HW] Set Throttle Cap: %.1f%%\n", ratio_0_1 * 100.0f);
  // Real implementation would write to MSRs or /sys/devices/system/cpu/...
  return true;
}

bool hw_seal_archive(bool on) {
  // "Sealing" implies hardware write-protect, TPM locking, or immutable file
  // attributes
  printf("🔒 [HW] Archive Seal: %s\n", on ? "ENGAGED" : "RELEASED");
  return true;
}
