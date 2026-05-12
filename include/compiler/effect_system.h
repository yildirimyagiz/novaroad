// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA EFFECT SYSTEM BACKEND  v2.0                                      ║
// ║  Algebraic Effects and Effect Handlers                                 ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_EFFECT_SYSTEM_H
#define NOVA_EFFECT_SYSTEM_H

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

typedef struct nova_effect_system nova_effect_system_t;
typedef struct nova_effect_handler nova_effect_handler_t;
typedef struct nova_effect nova_effect_t;

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Create a new algebraic effect system context
nova_effect_system_t *nova_effect_system_create(void);

/// Destroy effect system context
void nova_effect_system_destroy(nova_effect_system_t *system);

/// Perform an effectful operation
int nova_effect_system_perform_operation(nova_effect_system_t *system,
                                         nova_effect_handler_t *handler, const char *operation_name,
                                         void **args, size_t arg_count);

/// Create an effect handler
nova_effect_handler_t *nova_effect_system_create_handler(nova_effect_system_t *system,
                                                         const char *effect_name,
                                                         nova_stmt_t *handler_body);

/// Destroy effect handler
void nova_effect_handler_destroy(nova_effect_handler_t *handler);

/// Get the number of built-in effects
size_t nova_effect_system_get_builtin_effect_count(nova_effect_system_t *system);

/// Get built-in effect name by index
const char *nova_effect_system_get_builtin_effect_name(nova_effect_system_t *system, size_t index);

#endif /* NOVA_EFFECT_SYSTEM_H */
