#ifndef NOVA_DETERMINISTIC_REDUCTION_H
#define NOVA_DETERMINISTIC_REDUCTION_H

#include "../ml/nova_tensor.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA DETERMINISTIC REDUCTION - Order-Stable Math
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Performs a sum reduction with guaranteed bitwise reproducibility
float nova_deterministic_sum(NovaTensor *t);

// Standardizes FP accumulation to avoid non-associativity drift
void nova_standardize_accumulation(void);

#endif // NOVA_DETERMINISTIC_REDUCTION_H
