/**
 * @file test_pattern_semantic.c
 * @brief Pattern semantic analysis tests
 */

#include "compiler/pattern.h"
#include "compiler/semantic.h"
#include <stdio.h>
#include <assert.h>

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  ✓ PASS\n")

// Test 1: Pattern type checking
void test_pattern_type_checking() {
    TEST("Pattern type checking");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Wildcard matches any type
    Pattern *p1 = pattern_wildcard(0, 0);
    Type *int_ty = type_prim(TYPE_I64);
    assert(pattern_check_type(p1, int_ty, ctx));
    
    // Binding matches any type
    Pattern *p2 = pattern_binding("x", false, 0, 0);
    assert(pattern_check_type(p2, int_ty, ctx));
    
    // Check binding was added to symbol table
    Type *bound_type = symbol_table_lookup(ctx->symbols, "x");
    assert(bound_type != NULL);
    assert(types_equal(bound_type, int_ty));
    
    pattern_free(p1);
    pattern_free(p2);
    semantic_context_free(ctx);
    PASS();
}

// Test 2: Tuple pattern type checking
void test_tuple_pattern_types() {
    TEST("Tuple pattern type checking");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Create tuple type: (i64, f64, bool)
    Type *types[] = {
        type_prim(TYPE_I64),
        type_prim(TYPE_F64),
        type_prim(TYPE_BOOL),
    };
    Type *tuple_ty = type_tuple(types, 3);
    
    // Create matching pattern: (x, y, z)
    Pattern *patterns[] = {
        pattern_binding("x", false, 0, 0),
        pattern_binding("y", false, 0, 0),
        pattern_binding("z", false, 0, 0),
    };
    Pattern *tuple_pat = pattern_tuple(patterns, 3, 0, 0);
    
    assert(pattern_check_type(tuple_pat, tuple_ty, ctx));
    
    // Wrong arity should fail
    Pattern *wrong_patterns[] = {
        pattern_binding("a", false, 0, 0),
        pattern_binding("b", false, 0, 0),
    };
    Pattern *wrong_pat = pattern_tuple(wrong_patterns, 2, 0, 0);
    assert(!pattern_check_type(wrong_pat, tuple_ty, ctx));
    
    pattern_free(tuple_pat);
    pattern_free(wrong_pat);
    semantic_context_free(ctx);
    PASS();
}

// Test 3: Exhaustiveness checking
void test_exhaustiveness() {
    TEST("Exhaustiveness checking");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Create Option<i64> type
    Type *option_ty = type_enum("Option", /* ... */);
    
    // Exhaustive match: Some(_), None
    MatchArm arms[] = {
        {
            .pattern = pattern_variant("Some", 
                (Pattern*[]){pattern_wildcard(0, 0)}, 1, 0, 0),
            .guard = NULL,
            .body = /* ... */,
        },
        {
            .pattern = pattern_variant("None", NULL, 0, 0, 0),
            .guard = NULL,
            .body = /* ... */,
        },
    };
    
    MatchExpr *exhaustive = match_expr_create(/* scrutinee */, arms, 2);
    assert(match_check_exhaustive(exhaustive, ctx));
    
    // Non-exhaustive: only Some(_)
    MatchArm partial_arms[] = {
        {
            .pattern = pattern_variant("Some",
                (Pattern*[]){pattern_wildcard(0, 0)}, 1, 0, 0),
            .guard = NULL,
            .body = /* ... */,
        },
    };
    
    MatchExpr *non_exhaustive = match_expr_create(/* scrutinee */, partial_arms, 1);
    assert(!match_check_exhaustive(non_exhaustive, ctx));
    
    match_expr_free(exhaustive);
    match_expr_free(non_exhaustive);
    semantic_context_free(ctx);
    PASS();
}

// Test 4: Reachability checking
void test_reachability() {
    TEST("Reachability checking");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Unreachable pattern: wildcard followed by literal
    MatchArm arms[] = {
        {
            .pattern = pattern_wildcard(0, 0),  // Catches everything
            .guard = NULL,
            .body = /* ... */,
        },
        {
            .pattern = pattern_literal(/* 42 */, 0, 0),  // Unreachable!
            .guard = NULL,
            .body = /* ... */,
        },
    };
    
    MatchExpr *match = match_expr_create(/* scrutinee */, arms, 2);
    
    // Should detect unreachable pattern
    assert(!match_check_reachability(match, ctx));
    
    match_expr_free(match);
    semantic_context_free(ctx);
    PASS();
}

// Test 5: Or-pattern bindings
void test_or_pattern_bindings() {
    TEST("Or-pattern binding consistency");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Valid: both alternatives bind same variable
    Pattern *valid_alts[] = {
        pattern_binding("x", false, 0, 0),
        pattern_binding("x", false, 0, 0),
    };
    Pattern *valid_or = pattern_or(valid_alts, 2, 0, 0);
    assert(pattern_check_or_bindings(valid_or, ctx));
    
    // Invalid: different bindings
    Pattern *invalid_alts[] = {
        pattern_binding("x", false, 0, 0),
        pattern_binding("y", false, 0, 0),  // Different name!
    };
    Pattern *invalid_or = pattern_or(invalid_alts, 2, 0, 0);
    assert(!pattern_check_or_bindings(invalid_or, ctx));
    
    pattern_free(valid_or);
    pattern_free(invalid_or);
    semantic_context_free(ctx);
    PASS();
}

// Test 6: Guard type checking
void test_guard_types() {
    TEST("Guard expression type checking");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Guard must be boolean
    Expr *valid_guard = expr_binary(/* n < 0 */, OP_LT, /* ... */);
    Expr *invalid_guard = expr_literal_int(42, 0, 0);  // Not boolean!
    
    Type *guard_type1 = expr_infer_type(valid_guard, ctx);
    assert(guard_type1->kind == TYPE_BOOL);
    
    Type *guard_type2 = expr_infer_type(invalid_guard, ctx);
    assert(guard_type2->kind != TYPE_BOOL);
    
    semantic_context_free(ctx);
    PASS();
}

// Test 7: Full match analysis
void test_match_analysis() {
    TEST("Full match expression analysis");
    
    SemanticContext *ctx = semantic_context_new();
    
    // Create valid match expression
    // match x {
    //     0 => "zero",
    //     1 => "one",
    //     _ => "other",
    // }
    
    MatchExpr *match = /* ... create match ... */;
    
    bool result = match_analyze(match, ctx);
    assert(result == true);
    
    // Result type should be inferred
    assert(match->result_type != NULL);
    
    match_expr_free(match);
    semantic_context_free(ctx);
    PASS();
}

// Main test runner
int main() {
    printf("═══════════════════════════════════════════════\n");
    printf("  Pattern Semantic Analysis Tests\n");
    printf("═══════════════════════════════════════════════\n\n");
    
    test_pattern_type_checking();
    test_tuple_pattern_types();
    test_exhaustiveness();
    test_reachability();
    test_or_pattern_bindings();
    test_guard_types();
    test_match_analysis();
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  ✓ All semantic tests passed!\n");
    printf("═══════════════════════════════════════════════\n");
    
    return 0;
}
