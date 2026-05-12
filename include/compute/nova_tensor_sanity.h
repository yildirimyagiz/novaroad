#ifndef NOVA_TENSOR_SANITY_H
#define NOVA_TENSOR_SANITY_H

#include "../ml/nova_tensor.h"
#include <stdbool.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA TENSOR SANITY CHECK HEADER
 * ═══════════════════════════════════════════════════════════════════════════
 * Cheap, fast validation before expensive Gödel verification.
 */

// Basic tensor invariant check (NULL, shape, dimensions)
bool nova_tensor_sanity_check(NovaTensor *t);

#endif // NOVA_TENSOR_SANITY_H
