/**
 * @file effects.c
 * @brief Effect system implementation
 */

#include "compiler/types.h"
#include "compiler/ast.h"
#include "std/alloc.h"

/* Effect kinds */
typedef enum {
    EFFECT_PURE,        /* No side effects */
    EFFECT_IO,          /* I/O operations */
    EFFECT_ASYNC,       /* Asynchronous operations */
    EFFECT_UNSAFE,      /* Unsafe operations */
    EFFECT_THROWS,      /* Can throw exceptions */
} effect_kind_t;

typedef struct {
    effect_kind_t kind;
    bool present;
} effect_t;

typedef struct {
    effect_t effects[5];
    size_t num_effects;
} effect_set_t;

/* Check if function has effect */
static bool has_effect(effect_set_t *set, effect_kind_t kind)
{
    for (size_t i = 0; i < set->num_effects; i++) {
        if (set->effects[i].kind == kind && set->effects[i].present) {
            return true;
        }
    }
    return false;
}

/* Add effect to set */
static void add_effect(effect_set_t *set, effect_kind_t kind)
{
    if (set->num_effects < 5) {
        set->effects[set->num_effects].kind = kind;
        set->effects[set->num_effects].present = true;
        set->num_effects++;
    }
}

/* Infer effects for expression */
static void infer_effects(effect_set_t *set, nova_ast_node_t *expr)
{
    if (!expr) return;
    
    switch (expr->type) {
        case AST_CALL:
            /* TODO: Check if called function has effects */
            add_effect(set, EFFECT_IO);
            break;
        default:
            break;
    }
}

/* Check effect constraints */
int nova_check_effects(nova_ast_node_t *ast)
{
    if (!ast) return -1;
    
    effect_set_t effects = {0};
    infer_effects(&effects, ast);
    
    /* TODO: Verify effect constraints are satisfied */
    
    return 0;
}
