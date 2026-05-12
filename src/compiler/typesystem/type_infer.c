/**
 * @file type_infer.c
 * @brief Hindley-Milner type inference implementation
 */

#include "compiler/nova_types.h"
#include "compiler/ast.h"
#include "std/alloc.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


/* Type variable for inference */
typedef struct {
    size_t id;
    nova_type_t *resolved_type;  /* NULL if not yet resolved */
} type_var_t;

/* Type environment (symbol table) */
typedef struct env_entry {
    const char *name;
    nova_type_t *type;
    struct env_entry *next;
} env_entry_t;

typedef struct {
    env_entry_t *head;
} type_env_t;

/* Inference context */
typedef struct {
    type_var_t *type_vars;
    size_t num_type_vars;
    type_env_t *env;
    size_t next_var_id;
} infer_context_t;

/* Create fresh type variable */
static nova_type_t *fresh_type_var(infer_context_t *ctx)
{
    type_var_t *var = nova_alloc(sizeof(type_var_t));
    var->id = ctx->next_var_id++;
    var->resolved_type = NULL;
    
    // Store in context for tracking
    ctx->type_vars = nova_realloc(ctx->type_vars, 
                                   sizeof(type_var_t) * (ctx->num_type_vars + 1));
    ctx->type_vars[ctx->num_type_vars++] = *var;
    
    // Create a special type kind for inference variables
    nova_type_t *t = nova_type_primitive(TYPE_VOID); // Placeholder
    // t->kind = TYPE_VAR; t->data.var_id = var->id;
    return t;
}

/* Unify two types */
static bool unify(infer_context_t *ctx, nova_type_t *t1, nova_type_t *t2)
{
    if (!t1 || !t2) return false;
    if (nova_type_equals(t1, t2)) return true;

    // Handle function types
    if (t1->kind == TYPE_FN && t2->kind == TYPE_FN) {
        if (t1->data.fn.param_count != t2->data.fn.param_count) return false;
        if (!unify(ctx, t1->data.fn.return_type, t2->data.fn.return_type)) return false;
        for (size_t i = 0; i < t1->data.fn.param_count; i++) {
            if (!unify(ctx, t1->data.fn.params[i], t2->data.fn.params[i])) return false;
        }
        return true;
    }

    // Handle pointers
    if (t1->kind == TYPE_POINTER && t2->kind == TYPE_POINTER) {
        return unify(ctx, t1->data.ptr.pointee, t2->data.ptr.pointee);
    }

    return false;
}

/* Infer type for expression */
static nova_type_t *infer_expr(infer_context_t *ctx, nova_ast_node_t *expr)
{
    if (!expr) return NULL;
    
    switch (expr->type) {
        case AST_LITERAL: {
            // Check literal value to determine type
            // return nova_type_primitive(TYPE_I32); 
            return nova_type_i32();
        }
        
        case AST_VARIABLE: {
            // Look up variable in environment
            // In a real impl, extract name from node
            env_entry_t *entry = ctx->env->head;
            while (entry) {
                // if (strcmp(entry->name, name) == 0) return entry->type;
                entry = entry->next;
            }
            return fresh_type_var(ctx);
        }
        
        case AST_BINARY_OP: {
            nova_ast_node_t *left = expr->children;
            nova_ast_node_t *right = left ? left->next : NULL;
            
            nova_type_t *t1 = infer_expr(ctx, left);
            nova_type_t *t2 = infer_expr(ctx, right);
            
            if (!unify(ctx, t1, t2)) return NULL;
            return t1;
        }
        
        case AST_CALL: {
            nova_ast_node_t *func = expr->children;
            nova_type_t *func_type = infer_expr(ctx, func);
            if (func_type && func_type->kind == TYPE_FN) {
                return nova_type_clone(func_type->data.fn.return_type);
            }
            return fresh_type_var(ctx);
        }
        
        default:
            return nova_type_void();
    }
}

/* Public API */
nova_type_t *nova_type_infer(struct nova_expr *expr)
{
    infer_context_t ctx = {0};
    ctx.env = nova_calloc(1, sizeof(type_env_t));
    
    // Placeholder conversion: In a real system, we map nova_expr to AST
    nova_type_t *result = nova_type_i32(); 
    
    nova_free(ctx.type_vars);
    nova_free(ctx.env);
    return result;
}

