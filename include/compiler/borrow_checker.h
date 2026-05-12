#ifndef NOVA_BORROW_CHECKER_H
#define NOVA_BORROW_CHECKER_H

#include "ast.h"
#include "diagnostics.h"

// Borrow kinds
typedef enum nova_borrow_kind {
    BORROW_SHARED, // *T (immutable borrow)
    BORROW_MUT,    // *mut T (mutable borrow)
    BORROW_OWNED   // T (owned value)
} nova_borrow_kind_t;

// Borrow info for tracking individual borrows
typedef struct nova_borrow_info {
    nova_borrow_kind_t kind;
    char *var_name;   // variable being borrowed
    int scope_level;  // lexical scope depth
    nova_span_t span; // source location
    bool is_active;   // still valid
} nova_borrow_info_t;

// Borrow context for scope management
typedef struct nova_borrow_context {
    nova_borrow_info_t **borrows; // dynamic array
    int borrow_count;
    int borrow_capacity;
    int current_scope;
} nova_borrow_context_t;

// Main borrow checker
typedef struct nova_borrow_checker {
    nova_diag_collector_t *diag;
    nova_borrow_context_t *ctx;
} nova_borrow_checker_t;

// === API ===

// Lifecycle
nova_borrow_checker_t *nova_borrow_checker_create(nova_diag_collector_t *diag);
void nova_borrow_checker_destroy(nova_borrow_checker_t *bc);

// Scope management
void nova_borrow_enter_scope(nova_borrow_checker_t *bc);
void nova_borrow_exit_scope(nova_borrow_checker_t *bc);

// Borrow registration and validation
bool nova_borrow_register(nova_borrow_checker_t *bc, nova_borrow_kind_t kind, const char *var_name,
                          nova_span_t span);

bool nova_borrow_validate_use(nova_borrow_checker_t *bc, const char *var_name, nova_span_t span);

bool nova_borrow_check_program(nova_borrow_checker_t *bc, nova_program_t *program);

// Expression/statement checking
bool nova_borrow_check_expr(nova_borrow_checker_t *bc, nova_expr_t *expr);
bool nova_borrow_check_stmt(nova_borrow_checker_t *bc, nova_stmt_t *stmt);

// Type analysis helpers
nova_borrow_kind_t nova_type_to_borrow_kind(nova_type_t *type);

// Debug
void nova_borrow_debug_print(nova_borrow_checker_t *bc);

#endif // NOVA_BORROW_CHECKER_H
