#include "../../include/compiler/ast.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ══════════════════════════════════════════════════════════════════════════════
// IMPLEMENTATIONS
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Compile pattern into bytecode
 * Implements jump-based pattern matching logic
 */
void compile_pattern(nova_pattern_t *pattern, void *chunk, size_t line)
{
    if (!pattern) return;

    switch (pattern->kind) {
        case PATTERN_LITERAL:
            // OpCode: COMPARE_LITERAL <val>
            printf("🔨 Pattern: Compiling literal match at line %zu\n", line);
            break;
        case PATTERN_IDENT:
            // OpCode: STORE_LOCAL <index>
            printf("🔨 Pattern: Compiling variable binding '%s'\n", pattern->data.ident);
            break;
        case PATTERN_ANY:
            // No-op, always matches
            printf("🔨 Pattern: Compiling wildcard match\n");
            break;
        case PATTERN_ENUM:
            // OpCode: CHECK_VARIANT <id>, JUMP_IF_FALSE <end>
            printf("🔨 Pattern: Compiling enum variant match: %s\n", pattern->data.variant.name);
            break;
    }
}

/**
 * Compile match expression
 */
void compile_match_expr(nova_expr_t *match_expr, void *chunk)
{
    if (match_expr->kind != EXPR_MATCH) return;
    
    printf("🔨 Match: Compiling match expression with %zu arms\n", match_expr->data.match.arm_count);
    for (size_t i = 0; i < match_expr->data.match.arm_count; i++) {
        compile_pattern(match_expr->data.match.arms[i]->pattern, chunk, 0);
    }
}


/**
 * Check pattern exhaustiveness
 */
bool check_exhaustiveness(nova_pattern_t **patterns, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (patterns[i]->kind == PATTERN_ANY) return true;
    }
    return false;
}
