#ifndef NOVA_INVARIANT_ENGINE_H
#define NOVA_INVARIANT_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA INVARIANT ENGINE
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Protects system integrity via runtime invariants and safety rollbacks.
 */

typedef enum {
  INVARIANT_MEMORY_SAFETY,
  INVARIANT_RANGE_CHECK,
  INVARIANT_TYPE_INTEGRITY,
  INVARIANT_LOGIC_CONTINUITY,
  INVARIANT_NUMERICAL_STABILITY
} InvariantType;

typedef struct {
  InvariantType type;
  const char *expression;
  bool (*check_fn)(void *ctx);
  void *ctx;
} NovaConstraint;

typedef struct NovaInvariantEngine {
  NovaConstraint *constraints;
  size_t count;
  size_t capacity;

  // Rollback state
  void *safe_state_buffer;
  size_t state_size;
} NovaInvariantEngine;

NovaInvariantEngine *nova_invariant_init(size_t state_reserve_size);
void nova_invariant_shutdown(NovaInvariantEngine *ie);

// Enforcement
void nova_invariant_register(NovaInvariantEngine *ie, NovaConstraint c);
bool nova_invariant_check_all(NovaInvariantEngine *ie);

// Safety actions
void nova_invariant_checkpoint(NovaInvariantEngine *ie,
                                 void *current_state);
void nova_invariant_rollback(NovaInvariantEngine *ie, void *current_state);

// Undefined state detection
void nova_invariant_detect_anomaly(NovaInvariantEngine *ie,
                                     void *sys_context);

#endif // NOVA_INVARIANT_ENGINE_H
