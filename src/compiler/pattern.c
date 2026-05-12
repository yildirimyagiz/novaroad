#include <stdbool.h>

/**
 * @file pattern.c
 * @brief Pattern matching implementation
 */

#include "compiler/nova_pattern.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

Pattern *pattern_wildcard(size_t line, size_t col)
{
    Pattern *p = malloc(sizeof(Pattern));
    p->kind = PATTERN_WILDCARD;
    p->line = line;
    p->column = col;
    p->expected_type = NULL;
    return p;
}

Pattern *pattern_literal(Expr *value, size_t line, size_t col)
{
    Pattern *p = malloc(sizeof(Pattern));
    p->kind = PATTERN_LITERAL;
    p->line = line;
    p->column = col;
    p->expected_type = NULL;
    p->data.literal.value = value;
    return p;
}

Pattern *pattern_binding(char *name, bool is_mut, size_t line, size_t col)
{
    Pattern *p = malloc(sizeof(Pattern));
    p->kind = PATTERN_BINDING;
    p->line = line;
    p->column = col;
    p->expected_type = NULL;
    p->data.binding.name = strdup(name);
    p->data.binding.is_mutable = is_mut;
    return p;
}

Pattern *pattern_tuple(Pattern **patterns, size_t count, size_t line, size_t col)
{
    Pattern *p = malloc(sizeof(Pattern));
    p->kind = PATTERN_TUPLE;
    p->line = line;
    p->column = col;
    p->expected_type = NULL;
    p->data.tuple.patterns = patterns;
    p->data.tuple.count = count;
    return p;
}

Pattern *pattern_variant(char *name, Pattern **args, size_t arg_count, size_t line, size_t col)
{
    Pattern *p = malloc(sizeof(Pattern));
    p->kind = PATTERN_VARIANT;
    p->line = line;
    p->column = col;
    p->expected_type = NULL;
    p->data.variant.variant_name = strdup(name);
    p->data.variant.args = args;
    p->data.variant.arg_count = arg_count;
    return p;
}

Pattern *pattern_or(Pattern **alternatives, size_t count, size_t line, size_t col)
{
    Pattern *p = malloc(sizeof(Pattern));
    p->kind = PATTERN_OR;
    p->line = line;
    p->column = col;
    p->expected_type = NULL;
    p->data.or_pattern.alternatives = alternatives;
    p->data.or_pattern.count = count;
    return p;
}

void pattern_free(Pattern *pattern)
{
    if (!pattern) return;
    
    switch (pattern->kind) {
    case PATTERN_BINDING:
        free(pattern->data.binding.name);
        break;
        
    case PATTERN_TUPLE:
        for (size_t i = 0; i < pattern->data.tuple.count; i++) {
            pattern_free(pattern->data.tuple.patterns[i]);
        }
        free(pattern->data.tuple.patterns);
        break;
        
    case PATTERN_VARIANT:
        free(pattern->data.variant.variant_name);
        for (size_t i = 0; i < pattern->data.variant.arg_count; i++) {
            pattern_free(pattern->data.variant.args[i]);
        }
        free(pattern->data.variant.args);
        break;
        
    case PATTERN_OR:
        for (size_t i = 0; i < pattern->data.or_pattern.count; i++) {
            pattern_free(pattern->data.or_pattern.alternatives[i]);
        }
        free(pattern->data.or_pattern.alternatives);
        break;
        
    default:
        break;
    }
    
    free(pattern);
}

// ══════════════════════════════════════════════════════════════════════════════
// MATCH ARM CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

MatchArm *match_arm_create(Pattern *pattern, Expr *guard, Expr *body)
{
    MatchArm *arm = malloc(sizeof(MatchArm));
    arm->pattern = pattern;
    arm->guard = guard;
    arm->body = body;
    return arm;
}

void match_arm_free(MatchArm *arm)
{
    if (!arm) return;
    pattern_free(arm->pattern);
    // Note: Expr free handled elsewhere
    free(arm);
}

// ══════════════════════════════════════════════════════════════════════════════
// MATCH EXPRESSION CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

MatchExpr *match_expr_create(Expr *scrutinee, MatchArm *arms, size_t arm_count)
{
    MatchExpr *expr = malloc(sizeof(MatchExpr));
    expr->scrutinee = scrutinee;
    expr->arms = arms;
    expr->arm_count = arm_count;
    expr->result_type = NULL;
    return expr;
}

void match_expr_free(MatchExpr *expr)
{
    if (!expr) return;
    for (size_t i = 0; i < expr->arm_count; i++) {
        match_arm_free(&expr->arms[i]);
    }
    free(expr->arms);
    free(expr);
}

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN UTILITIES
// ══════════════════════════════════════════════════════════════════════════════

bool pattern_has_bindings(Pattern *pattern)
{
    switch (pattern->kind) {
    case PATTERN_WILDCARD:
    case PATTERN_LITERAL:
        return false;
        
    case PATTERN_BINDING:
        return true;
        
    case PATTERN_TUPLE:
        for (size_t i = 0; i < pattern->data.tuple.count; i++) {
            if (pattern_has_bindings(pattern->data.tuple.patterns[i]))
                return true;
        }
        return false;
        
    case PATTERN_VARIANT:
        for (size_t i = 0; i < pattern->data.variant.arg_count; i++) {
            if (pattern_has_bindings(pattern->data.variant.args[i]))
                return true;
        }
        return false;
        
    case PATTERN_OR:
        // All alternatives should have same bindings
        if (pattern->data.or_pattern.count > 0)
            return pattern_has_bindings(pattern->data.or_pattern.alternatives[0]);
        return false;
        
    default:
        return false;
    }
}

bool pattern_is_irrefutable(Pattern *pattern)
{
    switch (pattern->kind) {
    case PATTERN_WILDCARD:
    case PATTERN_BINDING:
        return true;
        
    case PATTERN_TUPLE:
        for (size_t i = 0; i < pattern->data.tuple.count; i++) {
            if (!pattern_is_irrefutable(pattern->data.tuple.patterns[i]))
                return false;
        }
        return true;
        
    case PATTERN_LITERAL:
    case PATTERN_VARIANT:
    case PATTERN_OR:
        return false;
        
    default:
        return false;
    }
}

void pattern_print(Pattern *pattern)
{
    switch (pattern->kind) {
    case PATTERN_WILDCARD:
        printf("_");
        break;
        
    case PATTERN_BINDING:
        printf("%s", pattern->data.binding.name);
        break;
        
    case PATTERN_LITERAL:
        printf("<literal>");
        break;
        
    case PATTERN_TUPLE:
        printf("(");
        for (size_t i = 0; i < pattern->data.tuple.count; i++) {
            if (i > 0) printf(", ");
            pattern_print(pattern->data.tuple.patterns[i]);
        }
        printf(")");
        break;
        
    case PATTERN_VARIANT:
        printf("%s", pattern->data.variant.variant_name);
        if (pattern->data.variant.arg_count > 0) {
            printf("(");
            for (size_t i = 0; i < pattern->data.variant.arg_count; i++) {
                if (i > 0) printf(", ");
                pattern_print(pattern->data.variant.args[i]);
            }
            printf(")");
        }
        break;
        
    case PATTERN_OR:
        for (size_t i = 0; i < pattern->data.or_pattern.count; i++) {
            if (i > 0) printf(" | ");
            pattern_print(pattern->data.or_pattern.alternatives[i]);
        }
        break;
        
    default:
        printf("<unknown>");
    }
}
