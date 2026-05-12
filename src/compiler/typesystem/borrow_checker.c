/**
 * Nova Borrow Checker - Full Implementation
 *
 * Implements Rust-style ownership and borrowing rules for memory safety.
 */

#include "compiler/borrow_checker.h"
#include "compiler/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// Context Management
// ═══════════════════════════════════════════════════════════════════════════

static nova_borrow_context_t *borrow_context_create(void)
{
    nova_borrow_context_t *ctx = (nova_borrow_context_t *) malloc(sizeof(nova_borrow_context_t));
    ctx->borrows = NULL;
    ctx->borrow_count = 0;
    ctx->borrow_capacity = 0;
    ctx->current_scope = 0;
    return ctx;
}

static void borrow_context_destroy(nova_borrow_context_t *ctx)
{
    if (!ctx)
        return;

    for (int i = 0; i < ctx->borrow_count; i++) {
        if (ctx->borrows[i]) {
            free(ctx->borrows[i]->var_name);
            free(ctx->borrows[i]);
        }
    }
    free(ctx->borrows);
    free(ctx);
}

// ═══════════════════════════════════════════════════════════════════════════
// Borrow Checker Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

nova_borrow_checker_t *nova_borrow_checker_create(nova_diag_collector_t *diag)
{
    nova_borrow_checker_t *bc = (nova_borrow_checker_t *) malloc(sizeof(nova_borrow_checker_t));
    bc->diag = diag;
    bc->ctx = borrow_context_create();
    return bc;
}

void nova_borrow_checker_destroy(nova_borrow_checker_t *bc)
{
    if (!bc)
        return;
    borrow_context_destroy(bc->ctx);
    free(bc);
}

// ═══════════════════════════════════════════════════════════════════════════
// Scope Management
// ═══════════════════════════════════════════════════════════════════════════

void nova_borrow_enter_scope(nova_borrow_checker_t *bc)
{
    if (!bc || !bc->ctx)
        return;
    bc->ctx->current_scope++;
}

void nova_borrow_exit_scope(nova_borrow_checker_t *bc)
{
    if (!bc || !bc->ctx)
        return;

    // Invalidate all borrows in current scope
    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        if (bc->ctx->borrows[i]->scope_level >= bc->ctx->current_scope) {
            bc->ctx->borrows[i]->is_active = false;
        }
    }

    bc->ctx->current_scope--;
}

// ═══════════════════════════════════════════════════════════════════════════
// Borrow Registration and Validation
// ═══════════════════════════════════════════════════════════════════════════

bool nova_borrow_register(nova_borrow_checker_t *bc, nova_borrow_kind_t kind, const char *var_name,
                          nova_span_t span)
{
    if (!bc || !bc->ctx || !var_name)
        return false;

    // Check for conflicting borrows
    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        nova_borrow_info_t *existing = bc->ctx->borrows[i];

        if (!existing->is_active)
            continue;
        if (strcmp(existing->var_name, var_name) != 0)
            continue;

        // Rule: Cannot have mutable borrow while any other borrow exists
        if (kind == BORROW_MUT || existing->kind == BORROW_MUT) {
            nova_diag_error(bc->diag, span,
                            "Cannot borrow as mutable because it is also borrowed as immutable");
            return false;
        }
    }

    // Allocate space if needed
    if (bc->ctx->borrow_count >= bc->ctx->borrow_capacity) {
        int new_cap = bc->ctx->borrow_capacity == 0 ? 8 : bc->ctx->borrow_capacity * 2;
        bc->ctx->borrows = (nova_borrow_info_t **) realloc(
            bc->ctx->borrows, sizeof(nova_borrow_info_t *) * (size_t) new_cap);
        bc->ctx->borrow_capacity = new_cap;
    }

    // Register new borrow
    nova_borrow_info_t *borrow = (nova_borrow_info_t *) malloc(sizeof(nova_borrow_info_t));
    borrow->kind = kind;
    borrow->var_name = strdup(var_name);
    borrow->scope_level = bc->ctx->current_scope;
    borrow->span = span;
    borrow->is_active = true;

    bc->ctx->borrows[bc->ctx->borrow_count++] = borrow;
    return true;
}

bool nova_borrow_validate_use(nova_borrow_checker_t *bc, const char *var_name, nova_span_t span)
{
    if (!bc || !bc->ctx || !var_name)
        return false;

    // Check if variable has been moved
    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        nova_borrow_info_t *borrow = bc->ctx->borrows[i];

        if (!borrow->is_active)
            continue;
        if (strcmp(borrow->var_name, var_name) != 0)
            continue;

        if (borrow->kind == BORROW_OWNED) {
            // Value was moved, cannot use
            nova_diag_error(bc->diag, span, "Use of moved value");
            return false;
        }
    }

    return true;
}

static bool is_copy_type(nova_type_t *type)
{
    if (!type)
        return true;
    switch (type->kind) {
    case TYPE_I32:
    case TYPE_I64:
    case TYPE_F32:
    case TYPE_F64:
    case TYPE_U8:
    case TYPE_USIZE:
    case TYPE_BOOL:
    case TYPE_STR:
    case TYPE_PTR:
    case TYPE_PTR_MUT:
    case TYPE_POINTER:
        return true;
    default:
        return false;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Expression Checking
// ═══════════════════════════════════════════════════════════════════════════

bool nova_borrow_check_expr(nova_borrow_checker_t *bc, nova_expr_t *expr)
{
    if (!bc || !expr)
        return true;

    switch (expr->kind) {
    case EXPR_IDENT:
        // Check if variable is valid to use
        return nova_borrow_validate_use(bc, expr->data.ident, expr->span);

    case EXPR_ADDR_OF:
        // Creating a borrow (&x)
        if (expr->data.addr_of->kind == EXPR_IDENT) {
            nova_borrow_kind_t kind = BORROW_SHARED;
            return nova_borrow_register(bc, kind, expr->data.addr_of->data.ident, expr->span);
        }
        break;

    case EXPR_DEREF:
        // Dereferencing (*x) - check if borrow is valid
        return nova_borrow_check_expr(bc, expr->data.deref);

    case EXPR_ASSIGN:
        // Assignment (x = y)
        if (!nova_borrow_check_expr(bc, expr->data.assign.value))
            return false;

        // If RHS is a move type, mark it as moved
        if (expr->data.assign.value->kind == EXPR_IDENT) {
            if (!is_copy_type(expr->data.assign.value->type)) {
                nova_borrow_register(bc, BORROW_OWNED, expr->data.assign.value->data.ident,
                                     expr->data.assign.value->span);
            }
        }

        // LHS must be valid
        return nova_borrow_validate_use(bc, expr->data.assign.name, expr->span);

    case EXPR_BINARY:
        if (!nova_borrow_check_expr(bc, expr->data.binary.left))
            return false;
        return nova_borrow_check_expr(bc, expr->data.binary.right);

    case EXPR_CALL:
        // Function calls - check all arguments
        for (size_t i = 0; i < expr->data.call.arg_count; i++) {
            if (!nova_borrow_check_expr(bc, expr->data.call.args[i]))
                return false;

            // If arg is a move type, mark it as moved
            if (expr->data.call.args[i]->kind == EXPR_IDENT) {
                if (!is_copy_type(expr->data.call.args[i]->type)) {
                    nova_borrow_register(bc, BORROW_OWNED, expr->data.call.args[i]->data.ident,
                                         expr->data.call.args[i]->span);
                }
            }
        }
        if (!nova_borrow_check_expr(bc, expr->data.call.func))
            return false;
        break;

    case EXPR_FIELD_ACCESS:
        return nova_borrow_check_expr(bc, expr->data.field_access.object);

    case EXPR_HEAP_NEW:
        // Heap allocation - check constructor args
        for (size_t i = 0; i < expr->data.heap_new.arg_count; i++) {
            if (!nova_borrow_check_expr(bc, expr->data.heap_new.args[i]))
                return false;
        }
        break;

    case EXPR_STRUCT_INIT:
        // Struct initialization - check field values
        for (size_t i = 0; i < expr->data.struct_init.field_count; i++) {
            if (!nova_borrow_check_expr(bc, expr->data.struct_init.fields[i].value))
                return false;

            // If field value is a move type, mark it as moved
            if (expr->data.struct_init.fields[i].value->kind == EXPR_IDENT) {
                if (!is_copy_type(expr->data.struct_init.fields[i].value->type)) {
                    nova_borrow_register(bc, BORROW_OWNED,
                                         expr->data.struct_init.fields[i].value->data.ident,
                                         expr->data.struct_init.fields[i].value->span);
                }
            }
        }
        break;

    // Literals don't need checking
    case EXPR_INT:
    case EXPR_FLOAT:
    case EXPR_STR:
    case EXPR_BOOL:
        break;

    default:
        break;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Statement Checking
// ═══════════════════════════════════════════════════════════════════════════

bool nova_borrow_check_stmt(nova_borrow_checker_t *bc, nova_stmt_t *stmt)
{
    if (!bc || !stmt)
        return true;

    switch (stmt->kind) {
    case STMT_EXPR:
        return nova_borrow_check_expr(bc, stmt->data.expr);

    case STMT_VAR_DECL:
        // Variable declaration - check initializer
        if (stmt->data.var_decl.init) {
            if (!nova_borrow_check_expr(bc, stmt->data.var_decl.init))
                return false;

            // If initializer is a move type, mark it as moved
            if (stmt->data.var_decl.init->kind == EXPR_IDENT) {
                if (!is_copy_type(stmt->data.var_decl.init->type)) {
                    nova_borrow_register(bc, BORROW_OWNED, stmt->data.var_decl.init->data.ident,
                                         stmt->data.var_decl.init->span);
                }
            }
        }
        break;

    case STMT_RETURN:
    case STMT_YIELD: {
        nova_expr_t *expr =
            (stmt->kind == STMT_RETURN) ? stmt->data.return_expr : stmt->data.yield_stmt;
        if (expr) {
            if (!nova_borrow_check_expr(bc, expr))
                return false;

            // If return expression is a move type, mark it as moved
            if (expr->kind == EXPR_IDENT) {
                if (!is_copy_type(expr->type)) {
                    nova_borrow_register(bc, BORROW_OWNED, expr->data.ident, expr->span);
                }
            }
        }
        break;
    }

    case STMT_BLOCK:
        nova_borrow_enter_scope(bc);
        for (size_t i = 0; i < stmt->data.block.count; i++) {
            if (!nova_borrow_check_stmt(bc, stmt->data.block.statements[i])) {
                nova_borrow_exit_scope(bc);
                return false;
            }
        }
        nova_borrow_exit_scope(bc);
        break;

    case STMT_FN:
        // Function body
        nova_borrow_enter_scope(bc);
        if (stmt->data.fn_stmt.body) {
            if (!nova_borrow_check_stmt(bc, stmt->data.fn_stmt.body)) {
                nova_borrow_exit_scope(bc);
                return false;
            }
        }
        nova_borrow_exit_scope(bc);
        break;

    case STMT_CHECK:
        // if-else statement
        if (stmt->data.check_stmt.condition) {
            if (!nova_borrow_check_expr(bc, stmt->data.check_stmt.condition))
                return false;
        }
        if (stmt->data.check_stmt.then_branch) {
            if (!nova_borrow_check_stmt(bc, stmt->data.check_stmt.then_branch))
                return false;
        }
        if (stmt->data.check_stmt.else_branch) {
            if (!nova_borrow_check_stmt(bc, stmt->data.check_stmt.else_branch))
                return false;
        }
        break;

    case STMT_WHILE:
        if (stmt->data.while_stmt.condition) {
            if (!nova_borrow_check_expr(bc, stmt->data.while_stmt.condition))
                return false;
        }
        if (stmt->data.while_stmt.body) {
            if (!nova_borrow_check_stmt(bc, stmt->data.while_stmt.body))
                return false;
        }
        break;

    case STMT_HEAP_FREE:
        // Free statement - check pointer
        if (stmt->data.heap_free) {
            return nova_borrow_check_expr(bc, stmt->data.heap_free);
        }
        break;

    default:
        break;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Type Analysis
// ═══════════════════════════════════════════════════════════════════════════

nova_borrow_kind_t nova_type_to_borrow_kind(nova_type_t *type)
{
    if (!type)
        return BORROW_OWNED;

    switch (type->kind) {
    case TYPE_PTR:
        return BORROW_SHARED;
    case TYPE_PTR_MUT:
        return BORROW_MUT;
    default:
        return BORROW_OWNED;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Program-Level Checking
// ═══════════════════════════════════════════════════════════════════════════

bool nova_borrow_check_program(nova_borrow_checker_t *bc, nova_program_t *program)
{
    if (!bc || !program)
        return true;

    bool success = true;
    for (size_t i = 0; i < program->declaration_count; i++) {
        nova_top_level_t *decl = program->declarations[i];
        if (decl->kind == DECL_FUNCTION) {
            if (!nova_borrow_check_stmt(bc, (nova_stmt_t *) decl->data)) {
                success = false;
            }
        }
    }

    return success;
}

// ═══════════════════════════════════════════════════════════════════════════
// Debug
// ═══════════════════════════════════════════════════════════════════════════

void nova_borrow_debug_print(nova_borrow_checker_t *bc)
{
    if (!bc || !bc->ctx)
        return;

    printf("=== Borrow Checker State ===\n");
    printf("Scope level: %d\n", bc->ctx->current_scope);
    printf("Active borrows: %d\n", bc->ctx->borrow_count);

    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        nova_borrow_info_t *b = bc->ctx->borrows[i];
        const char *kind_str = b->kind == BORROW_SHARED ? "shared"
                               : b->kind == BORROW_MUT  ? "mutable"
                                                        : "owned";
        printf("  [%d] %s: %s (scope=%d, active=%d)\n", i, b->var_name, kind_str, b->scope_level,
               b->is_active);
    }
}
