#include "../../include/nova_fault_intelligence.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

NovaFaultIntelligence *nova_fault_init(void) {
  NovaFaultIntelligence *fi = calloc(1, sizeof(NovaFaultIntelligence));
  fi->capacity = 500;
  fi->history = calloc(fi->capacity, sizeof(NovaFaultReport));
  fi->real_time_monitoring = true;

  printf("🔍 Nova Fault Intelligence System Active\n");
  return fi;
}

void nova_fault_shutdown(NovaFaultIntelligence *fi) {
  if (!fi)
    return;
  free(fi->history);
  free(fi);
}

void nova_fault_report(NovaFaultIntelligence *fi,
                         NovaFaultReport report) {
  if (fi->count >= fi->capacity) {
    // Simple ring buffer logic or realloc
    memmove(&fi->history[0], &fi->history[1],
            (fi->count - 1) * sizeof(NovaFaultReport));
    fi->count--;
  }

  fi->history[fi->count++] = report;

  if (report.severity >= 8) {
    printf("🚨 CRITICAL FAULT DETECTED: %s (Severity: %d)\n",
           report.description, report.severity);
  }
}

void nova_fault_analyze_concurrency(NovaFaultIntelligence *fi,
                                      void *mutex_id, void *thread_id) {
  // Placeholder for race condition detection logic
  // In a full implementation, we'd track lock ordering etc.
}

void nova_fault_check_stability(NovaFaultIntelligence *fi, double value,
                                  const char *context) {
  if (isnan(value) || isinf(value)) {
    NovaFaultReport r;
    r.type = FAULT_NUMERICAL_INSTABILITY;
    r.severity = 9;
    r.timestamp = (uint64_t)time(NULL);
    snprintf(r.description, 255,
             "Numerical instability (NaN/Inf) detected in %s", context);
    nova_fault_report(fi, r);
  }
}

void nova_fault_dump_all(NovaFaultIntelligence *fi) {
  printf("\n🗒️ NOVA FAULT INTELLIGENCE LOG\n");
  printf("---------------------------------------------------------------------"
         "-----------\n");
  for (size_t i = 0; i < fi->count; i++) {
    NovaFaultReport *r = &fi->history[i];
    printf("[%llu] Type:%d Severity:%d - %s\n",
           (unsigned long long)r->timestamp, r->type, r->severity,
           r->description);
  }
  printf("---------------------------------------------------------------------"
         "-----------\n\n");
}
