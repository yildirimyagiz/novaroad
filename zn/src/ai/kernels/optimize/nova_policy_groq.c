/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_policy_groq.c — Groq AI Optimized Execution Permission Engine
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_policy.h"
#include "nova_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Groq AI: Enerji bazlı policy kararları
static double groq_policy_energy_threshold = 100.0; // Watt

const char *policy_mode_name(PolicyMode mode) {
  switch (mode) {
  case POLICY_STRICT:
    return "STRICT";
  case POLICY_GUARDED:
    return "GUARDED";
  case POLICY_PERMISSIVE:
    return "PERMISSIVE";
  case POLICY_DISABLED:
    return "DISABLED";
  default:
    return "UNKNOWN";
  }
}

const char *verdict_name(ExecutionVerdict verdict) {
  switch (verdict) {
  case VERDICT_ALLOW:
    return "ALLOW";
  case VERDICT_WARN:
    return "WARN";
  case VERDICT_DENY:
    return "DENY";
  case VERDICT_QUARANTINE:
    return "QUARANTINE";
  default:
    return "UNKNOWN";
  }
}

ExecutionVerdict nova_policy_decide(const char *kernel_name, PolicyMode mode,
                                     bool is_proven_safe, double energy_usage) {
  // Groq AI: Enerji kontrolü ekle
  if (energy_usage > groq_policy_energy_threshold) {
    printf("Groq AI: High energy usage %.2f, denying execution\n", energy_usage);
    return VERDICT_DENY;
  }

  switch (mode) {
  case POLICY_STRICT:
    return is_proven_safe ? VERDICT_ALLOW : VERDICT_DENY;

  case POLICY_GUARDED:
    return is_proven_safe ? VERDICT_ALLOW : VERDICT_WARN;

  case POLICY_PERMISSIVE:
    return VERDICT_ALLOW;

  case POLICY_DISABLED:
    return VERDICT_ALLOW;

  default:
    return VERDICT_DENY;
  }
}

bool nova_policy_check_kernel(const char *kernel_name, PolicyMode mode) {
  // Basitleştirilmiş: Her zaman izin ver, ama enerji kontrolü ile
  double mock_energy = 50.0; // Tahmini
  ExecutionVerdict v = nova_policy_decide(kernel_name, mode, true, mock_energy);
  return v == VERDICT_ALLOW;
}

// Groq AI: Çin/Hindistan için lokalizasyon policy'si
bool groq_policy_localize(const char *market) {
  if (strcmp(market, "China") == 0) {
    printf("Groq AI: Applying China localization policy\n");
    return true;
  } else if (strcmp(market, "India") == 0) {
    printf("Groq AI: Applying India localization policy\n");
    return true;
  }
  return false;
}
