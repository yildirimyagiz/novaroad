#include "../../include/nova_invariant_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaInvariantEngine *nova_invariant_init(size_t state_reserve_size) {
  NovaInvariantEngine *ie = calloc(1, sizeof(NovaInvariantEngine));
  ie->capacity = 128;
  ie->constraints = calloc(ie->capacity, sizeof(NovaConstraint));
  ie->state_size = state_reserve_size;
  ie->safe_state_buffer = malloc(state_reserve_size);

  printf("🛡️ Nova Invariant Engine Booted (Reserve: %zu bytes)\n",
         state_reserve_size);
  return ie;
}

void nova_invariant_shutdown(NovaInvariantEngine *ie) {
  if (!ie)
    return;
  free(ie->constraints);
  free(ie->safe_state_buffer);
  free(ie);
}

void nova_invariant_register(NovaInvariantEngine *ie, NovaConstraint c) {
  if (ie->count >= ie->capacity) {
    ie->capacity *= 2;
    ie->constraints =
        realloc(ie->constraints, ie->capacity * sizeof(NovaConstraint));
  }
  ie->constraints[ie->count++] = c;
}

bool nova_invariant_check_all(NovaInvariantEngine *ie) {
  for (size_t i = 0; i < ie->count; i++) {
    if (ie->constraints[i].check_fn &&
        !ie->constraints[i].check_fn(ie->constraints[i].ctx)) {
      printf("❌ INVARIANT VIOLATION: %s [Type: %d]\n",
             ie->constraints[i].expression, ie->constraints[i].type);
      return false;
    }
  }
  return true;
}

void nova_invariant_checkpoint(NovaInvariantEngine *ie,
                                 void *current_state) {
  if (!ie || !current_state)
    return;
  memcpy(ie->safe_state_buffer, current_state, ie->state_size);
}

void nova_invariant_rollback(NovaInvariantEngine *ie, void *current_state) {
  if (!ie || !current_state)
    return;
  printf("⚠️ Safety Rollback Triggered! Restoring last known good state...\n");
  memcpy(current_state, ie->safe_state_buffer, ie->state_size);
}

void nova_invariant_detect_anomaly(NovaInvariantEngine *ie,
                                     void *sys_context) {
  // Advanced anomaly detection: check for numerical instability or undefined
  // state
  if (!nova_invariant_check_all(ie)) {
    nova_invariant_rollback(ie, sys_context);
  }
}
