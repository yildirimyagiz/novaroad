/**
 * @file test_pattern_parser.c
 * @brief Pattern parser unit tests
 */

#include "compiler/parser.h"
#include "compiler/pattern.h"
#include "compiler/lexer.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  ✓ PASS\n")

// Helper: parse pattern from string
Pattern *parse_pattern_from_string(const char *source) {
    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    Pattern *pattern = parse_pattern(parser);
    return pattern;
}

// Test 1: Wildcard pattern
void test_wildcard_pattern() {
    TEST("Wildcard pattern");
    
    Pattern *p = parse_pattern_from_string("_");
    assert(p != NULL);
    assert(p->kind == PATTERN_WILDCARD);
    assert(pattern_is_irrefutable(p));
    assert(!pattern_has_bindings(p));
    
    pattern_free(p);
    PASS();
}

// Test 2: Literal patterns
void test_literal_patterns() {
    TEST("Literal patterns");
    
    // Integer literal
    Pattern *p1 = parse_pattern_from_string("42");
    assert(p1 != NULL);
    assert(p1->kind == PATTERN_LITERAL);
    assert(!pattern_is_irrefutable(p1));
    
    // String literal
    Pattern *p2 = parse_pattern_from_string("\"hello\"");
    assert(p2 != NULL);
    assert(p2->kind == PATTERN_LITERAL);
    
    // Boolean literal
    Pattern *p3 = parse_pattern_from_string("true");
    assert(p3 != NULL);
    assert(p3->kind == PATTERN_LITERAL);
    
    pattern_free(p1);
    pattern_free(p2);
    pattern_free(p3);
    PASS();
}

// Test 3: Binding pattern
void test_binding_pattern() {
    TEST("Binding pattern");
    
    Pattern *p = parse_pattern_from_string("x");
    assert(p != NULL);
    assert(p->kind == PATTERN_BINDING);
    assert(strcmp(p->data.binding.name, "x") == 0);
    assert(pattern_is_irrefutable(p));
    assert(pattern_has_bindings(p));
    
    pattern_free(p);
    PASS();
}

// Test 4: Tuple pattern
void test_tuple_pattern() {
    TEST("Tuple pattern");
    
    Pattern *p = parse_pattern_from_string("(a, b, c)");
    assert(p != NULL);
    assert(p->kind == PATTERN_TUPLE);
    assert(p->data.tuple.count == 3);
    assert(p->data.tuple.patterns[0]->kind == PATTERN_BINDING);
    assert(p->data.tuple.patterns[1]->kind == PATTERN_BINDING);
    assert(p->data.tuple.patterns[2]->kind == PATTERN_BINDING);
    assert(pattern_has_bindings(p));
    
    pattern_free(p);
    PASS();
}

// Test 5: Variant pattern
void test_variant_pattern() {
    TEST("Variant pattern");
    
    Pattern *p = parse_pattern_from_string("Some(x)");
    assert(p != NULL);
    assert(p->kind == PATTERN_VARIANT);
    assert(strcmp(p->data.variant.variant_name, "Some") == 0);
    assert(p->data.variant.arg_count == 1);
    assert(p->data.variant.args[0]->kind == PATTERN_BINDING);
    assert(pattern_has_bindings(p));
    
    pattern_free(p);
    PASS();
}

// Test 6: Or pattern
void test_or_pattern() {
    TEST("Or pattern");
    
    Pattern *p = parse_pattern_from_string("1 | 2 | 3");
    assert(p != NULL);
    assert(p->kind == PATTERN_OR);
    assert(p->data.or_pattern.count == 3);
    assert(p->data.or_pattern.alternatives[0]->kind == PATTERN_LITERAL);
    assert(p->data.or_pattern.alternatives[1]->kind == PATTERN_LITERAL);
    assert(p->data.or_pattern.alternatives[2]->kind == PATTERN_LITERAL);
    
    pattern_free(p);
    PASS();
}

// Test 7: Nested pattern
void test_nested_pattern() {
    TEST("Nested pattern");
    
    Pattern *p = parse_pattern_from_string("Some((x, y))");
    assert(p != NULL);
    assert(p->kind == PATTERN_VARIANT);
    assert(p->data.variant.arg_count == 1);
    assert(p->data.variant.args[0]->kind == PATTERN_TUPLE);
    assert(p->data.variant.args[0]->data.tuple.count == 2);
    
    pattern_free(p);
    PASS();
}

// Test 8: Match expression
void test_match_expression() {
    TEST("Match expression");
    
    const char *source = 
        "match x {\n"
        "    0 => \"zero\",\n"
        "    1 => \"one\",\n"
        "    _ => \"other\",\n"
        "}";
    
    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    Expr *expr = parse_match_expression(parser);
    
    assert(expr != NULL);
    assert(expr->kind == EXPR_MATCH);
    assert(expr->data.match_expr != NULL);
    assert(expr->data.match_expr->arm_count == 3);
    
    // Check first arm
    MatchArm *arm0 = &expr->data.match_expr->arms[0];
    assert(arm0->pattern->kind == PATTERN_LITERAL);
    assert(arm0->guard == NULL);
    assert(arm0->body != NULL);
    
    // Check last arm (wildcard)
    MatchArm *arm2 = &expr->data.match_expr->arms[2];
    assert(arm2->pattern->kind == PATTERN_WILDCARD);
    
    expr_free(expr);
    PASS();
}

// Test 9: Match with guards
void test_match_with_guards() {
    TEST("Match with guard expressions");
    
    const char *source =
        "match x {\n"
        "    n if n < 0 => \"negative\",\n"
        "    n if n == 0 => \"zero\",\n"
        "    _ => \"positive\",\n"
        "}";
    
    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    Expr *expr = parse_match_expression(parser);
    
    assert(expr != NULL);
    assert(expr->data.match_expr->arm_count == 3);
    
    // Check guards
    assert(expr->data.match_expr->arms[0].guard != NULL);
    assert(expr->data.match_expr->arms[1].guard != NULL);
    assert(expr->data.match_expr->arms[2].guard == NULL);
    
    expr_free(expr);
    PASS();
}

// Main test runner
int main() {
    printf("═══════════════════════════════════════════════\n");
    printf("  Pattern Parser Unit Tests\n");
    printf("═══════════════════════════════════════════════\n\n");
    
    test_wildcard_pattern();
    test_literal_patterns();
    test_binding_pattern();
    test_tuple_pattern();
    test_variant_pattern();
    test_or_pattern();
    test_nested_pattern();
    test_match_expression();
    test_match_with_guards();
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  ✓ All tests passed!\n");
    printf("═══════════════════════════════════════════════\n");
    
    return 0;
}
