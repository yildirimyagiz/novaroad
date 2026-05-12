// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA DEPENDENT TYPES BACKEND  v2.0                                    ║
// ║  Type-Level Computation and Proof Objects                              ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_DEPENDENT_TYPES_H
#define NOVA_DEPENDENT_TYPES_H

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

typedef struct nova_dependent_type_system nova_dependent_type_system_t;
typedef struct nova_dependent_type nova_dependent_t;

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Create a new Pi type (dependent function type)
nova_type_t *nova_dependent_type_create_pi(const char *param_name, nova_type_t *param_type,
                                           nova_type_t *return_type);

/// Create a new dependent type system context
nova_dependent_type_system_t *nova_dependent_type_system_create(void);

/// Destroy dependent type system context
void nova_dependent_type_system_destroy(nova_dependent_type_system_t *system);

/// Lookup a type in the dependent type system
nova_type_t *nova_dependent_type_system_lookup(nova_dependent_type_system_t *system,
                                               const char *name);

/// Get the number of proofs in the system
size_t nova_dependent_type_system_proof_count(nova_dependent_type_system_t *system);

#endif /* NOVA_DEPENDENT_TYPES_H */
