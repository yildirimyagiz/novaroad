#ifndef NOVA_FAULT_INTELLIGENCE_H
#define NOVA_FAULT_INTELLIGENCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA FAULT INTELLIGENCE SYSTEM
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  FAULT_RACE_CONDITION,
  FAULT_NUMERICAL_INSTABILITY,
  FAULT_LOGIC_DRIFT,
  FAULT_RESOURCE_LEAK,
  FAULT_UNDEFINED_BEHAVIOR
} NovaFaultType;

typedef struct {
  NovaFaultType type;
  char description[256];
  uint64_t timestamp;
  void *stack_trace[16];
  int severity; // 0-10
} NovaFaultReport;

typedef struct NovaFaultIntelligence {
  NovaFaultReport *history;
  size_t count;
  size_t capacity;

  bool real_time_monitoring;
} NovaFaultIntelligence;

NovaFaultIntelligence *nova_fault_init(void);
void nova_fault_shutdown(NovaFaultIntelligence *fi);

// Detection API
void nova_fault_report(NovaFaultIntelligence *fi, NovaFaultReport report);
void nova_fault_analyze_concurrency(NovaFaultIntelligence *fi,
                                      void *mutex_id, void *thread_id);
void nova_fault_check_stability(NovaFaultIntelligence *fi, double value,
                                  const char *context);

// Diagnostic API
void nova_fault_dump_all(NovaFaultIntelligence *fi);

#endif // NOVA_FAULT_INTELLIGENCE_H
