/**
 * @file semantic.h
 * @brief Semantic analyzer
 */

#ifndef NOVA_SEMANTIC_H
#define NOVA_SEMANTIC_H

#include "ast.h"
#include "module_registry.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// nova_semantic_t is defined in ast.h
// Forward declaration removed to avoid conflict

// Symbol table entry
typedef struct nova_symbol {
    char *name;
    nova_type_t *type;
    int index; // For local vars
    int depth; // Scope depth
    bool is_global;
    bool is_mutable; // For mutability tracking
    struct nova_symbol *next;
} nova_symbol_t;

/**
 * Create semantic analyzer
 */
nova_semantic_t *nova_semantic_create(nova_program_t *ast);

/**
 * Run semantic analysis
 */
bool nova_semantic_analyze(nova_semantic_t *semantic);

/* Module registry integration */
void nova_semantic_set_module_registry(nova_module_registry_t *reg);
nova_module_registry_t *nova_semantic_get_module_registry(void);

/**
 * Get error message
 */
const char *nova_semantic_get_error(nova_semantic_t *semantic);

/**
 * Destroy semantic analyzer
 */
void nova_semantic_destroy(nova_semantic_t *semantic);

/**
 * Symbol table access for codegen
 */
nova_symbol_t *nova_semantic_lookup_variable(nova_semantic_t *semantic, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_SEMANTIC_H */
