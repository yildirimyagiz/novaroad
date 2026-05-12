/**
 * @file nova_pattern.h
 * @brief Pattern matching AST and utilities
 */

#ifndef NOVA_PATTERN_H
#define NOVA_PATTERN_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct Type Type;
typedef struct Expr Expr;

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN TYPES
// ══════════════════════════════════════════════════════════════════════════════

typedef enum {
    PATTERN_WILDCARD,    // _
    PATTERN_LITERAL,     // 42, "hello", true
    PATTERN_BINDING,     // x, name
    PATTERN_TUPLE,       // (a, b, c)
    PATTERN_ARRAY,       // [head, ..tail]
    PATTERN_VARIANT,     // Some(x), Ok(val)
    PATTERN_RECORD,      // Point { x, y }
    PATTERN_OR,          // a | b | c
    PATTERN_REST,        // .. (rest pattern)
} PatternKind;

typedef struct Pattern {
    PatternKind kind;
    size_t line;
    size_t column;
    Type *expected_type;  // Type this pattern should match
    
    union {
        // PATTERN_LITERAL
        struct {
            Expr *value;  // Literal expression
        } literal;
        
        // PATTERN_BINDING
        struct {
            char *name;
            bool is_mutable;
        } binding;
        
        // PATTERN_TUPLE
        struct {
            struct Pattern **patterns;
            size_t count;
        } tuple;
        
        // PATTERN_ARRAY
        struct {
            struct Pattern **patterns;
            size_t count;
            bool has_rest;  // true if contains ..
        } array;
        
        // PATTERN_VARIANT
        struct {
            char *variant_name;
            struct Pattern **args;  // Constructor arguments
            size_t arg_count;
        } variant;
        
        // PATTERN_RECORD
        struct {
            char *type_name;
            char **field_names;
            struct Pattern **field_patterns;
            size_t field_count;
        } record;
        
        // PATTERN_OR
        struct {
            struct Pattern **alternatives;
            size_t count;
        } or_pattern;
    } data;
} Pattern;

// ══════════════════════════════════════════════════════════════════════════════
// MATCH EXPRESSIONS
// ══════════════════════════════════════════════════════════════════════════════

typedef struct {
    Pattern *pattern;
    Expr *guard;      // Optional guard expression (if condition)
    Expr *body;       // Expression to execute if matched
} MatchArm;

typedef struct {
    Expr *scrutinee;  // Value being matched
    MatchArm *arms;
    size_t arm_count;
    Type *result_type;  // Type of the match expression
} MatchExpr;

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

Pattern *pattern_wildcard(size_t line, size_t col);
Pattern *pattern_literal(Expr *value, size_t line, size_t col);
Pattern *pattern_binding(char *name, bool is_mut, size_t line, size_t col);
Pattern *pattern_tuple(Pattern **patterns, size_t count, size_t line, size_t col);
Pattern *pattern_variant(char *name, Pattern **args, size_t arg_count, size_t line, size_t col);
Pattern *pattern_or(Pattern **alternatives, size_t count, size_t line, size_t col);

void pattern_free(Pattern *pattern);

// ══════════════════════════════════════════════════════════════════════════════
// MATCH ARM CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

MatchArm *match_arm_create(Pattern *pattern, Expr *guard, Expr *body);
void match_arm_free(MatchArm *arm);

// ══════════════════════════════════════════════════════════════════════════════
// MATCH EXPRESSION CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

MatchExpr *match_expr_create(Expr *scrutinee, MatchArm *arms, size_t arm_count);
void match_expr_free(MatchExpr *expr);

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN UTILITIES
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Check if pattern binds any variables
 */
bool pattern_has_bindings(Pattern *pattern);

/**
 * Get all variable names bound by pattern
 */
char **pattern_get_bindings(Pattern *pattern, size_t *count);

/**
 * Check if pattern is irrefutable (always matches)
 */
bool pattern_is_irrefutable(Pattern *pattern);

/**
 * Check if two patterns overlap (can match same value)
 */
bool patterns_overlap(Pattern *a, Pattern *b);

/**
 * Print pattern for debugging
 */
void pattern_print(Pattern *pattern);

#endif // NOVA_PATTERN_H
