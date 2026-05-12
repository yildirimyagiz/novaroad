/**
 * @file nova_error.h
 * @brief Error handling AST and types
 */

#ifndef NOVA_ERROR_H
#define NOVA_ERROR_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Type Type;

// ══════════════════════════════════════════════════════════════════════════════
// RESULT TYPE
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Result<T, E> - represents success (Ok) or failure (Err)
 */
typedef enum {
    RESULT_OK,
    RESULT_ERR,
} ResultVariant;

typedef struct {
    ResultVariant variant;
    union {
        void *ok_value;   // T
        void *err_value;  // E
    } data;
    Type *ok_type;   // Type T
    Type *err_type;  // Type E
} ResultValue;

// ══════════════════════════════════════════════════════════════════════════════
// ERROR TYPES
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Standard error categories
 */
typedef enum {
    ERROR_IO,           // File/network I/O errors
    ERROR_PARSE,        // Parsing errors
    ERROR_RUNTIME,      // Runtime errors
    ERROR_VALIDATION,   // Validation errors
    ERROR_CUSTOM,       // User-defined errors
} ErrorCategory;

/**
 * Error value
 */
typedef struct {
    ErrorCategory category;
    char *message;
    size_t line;
    size_t column;
    char *file;
    struct Error *cause;  // Nested error (chain)
} Error;

// ══════════════════════════════════════════════════════════════════════════════
// TRY/CATCH EXPRESSIONS
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Catch handler
 */
typedef struct {
    char *error_var;       // Variable to bind error (e.g., "e")
    Type *error_type;      // Type of error to catch (NULL = catch all)
    Stmt *handler;         // Handler block
} CatchHandler;

/**
 * Try expression
 */
typedef struct {
    Stmt *try_block;
    CatchHandler *handlers;
    size_t handler_count;
    Stmt *finally_block;   // Optional finally block
} TryExpr;

// ══════════════════════════════════════════════════════════════════════════════
// ERROR PROPAGATION (? OPERATOR)
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Error propagation expression (?)
 * Desugars to: match expr { Ok(v) => v, Err(e) => return Err(e) }
 */
typedef struct {
    Expr *inner;           // Expression that returns Result<T, E>
    Type *ok_type;         // Type T
    Type *err_type;        // Type E
} PropagateExpr;

// ══════════════════════════════════════════════════════════════════════════════
// PANIC/ABORT
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Panic expression (unrecoverable error)
 */
typedef struct {
    char *message;
    Expr *message_expr;    // Dynamic message
} PanicExpr;

// ══════════════════════════════════════════════════════════════════════════════
// CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

// Result constructors
ResultValue *result_ok(void *value, Type *ok_type, Type *err_type);
ResultValue *result_err(void *error, Type *ok_type, Type *err_type);
void result_free(ResultValue *result);

// Error constructors
Error *error_new(ErrorCategory category, const char *message);
Error *error_with_location(ErrorCategory category, const char *message,
                          size_t line, size_t col, const char *file);
Error *error_with_cause(ErrorCategory category, const char *message, Error *cause);
void error_free(Error *err);

// Try/Catch constructors
TryExpr *try_expr_new(Stmt *try_block);
void try_expr_add_handler(TryExpr *try_expr, CatchHandler *handler);
void try_expr_set_finally(TryExpr *try_expr, Stmt *finally_block);
void try_expr_free(TryExpr *try_expr);

CatchHandler *catch_handler_new(char *var, Type *type, Stmt *handler);
void catch_handler_free(CatchHandler *handler);

// Propagate constructor
PropagateExpr *propagate_expr_new(Expr *inner);
void propagate_expr_free(PropagateExpr *expr);

// Panic constructor
PanicExpr *panic_expr_new(const char *message);
PanicExpr *panic_expr_dynamic(Expr *message_expr);
void panic_expr_free(PanicExpr *expr);

// ══════════════════════════════════════════════════════════════════════════════
// UTILITIES
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Check if type is Result<T, E>
 */
bool type_is_result(Type *ty);

/**
 * Get Ok type from Result<T, E>
 */
Type *result_get_ok_type(Type *result_ty);

/**
 * Get Err type from Result<T, E>
 */
Type *result_get_err_type(Type *result_ty);

/**
 * Check if error types are compatible
 */
bool errors_compatible(Type *err1, Type *err2);

/**
 * Print error for debugging
 */
void error_print(Error *err);

/**
 * Format error message
 */
char *error_format(Error *err);

#endif // NOVA_ERROR_H
