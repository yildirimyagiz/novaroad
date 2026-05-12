/**
 * @file test_parser.c
 * @brief Unit tests for Nova Stage 0 Parser
 */

#include "compiler/lexer.h"
#include "compiler/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_parser_basic() {
    printf("--- Test: Parser Basic Expressions and Statements ---\n");
    
    // Simple mathematical expression
    const char *source = "2 + 3 * 4";
    nova_lexer_t *lexer = nova_lexer_create(source);
    assert(lexer != NULL);
    
    nova_parser_t *parser = nova_parser_create(lexer);
    assert(parser != NULL);
    
    nova_expr_t *expr = nova_parser_parse_expression(parser);
    assert(expr != NULL);
    assert(!nova_parser_had_error(parser));
    
    nova_expr_free(expr);
    nova_parser_destroy(parser);
    nova_lexer_destroy(lexer);
    
    printf("✓ Parser basic expression successful\n");
}

int main(void) {
    printf("=== Running Parser Unit Tests ===\n");
    test_parser_basic();
    printf("=== All Parser Tests Passed! ===\n");
    return 0;
}
