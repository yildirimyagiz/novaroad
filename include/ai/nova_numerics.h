#ifndef NOVA_NUMERICS_H
#define NOVA_NUMERICS_H

#include "../ml/nova_tensor.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA NUMERICAL INTELLIGENCE - Precision & Stability
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  float loss_scale;
  bool dynamic_scaling;
  bool auto_mixed_precision;
  bool nan_inf_prevention;
} NovaNumericsPolicy;

// Enforces numerical stability on a tensor
void nova_numerics_stabilize(NovaTensor *t, NovaNumericsPolicy policy);

// Decides the best precision for an operation
NovaDType nova_numerics_decide_precision(const char *op_id,
                                             NovaTensor **inputs, int count);

#endif // NOVA_NUMERICS_H
