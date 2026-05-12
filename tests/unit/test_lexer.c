/**
 * @file test_lexer.c
 * @brief Unit tests for Nova Stage 0 Lexer
 */

#include "compiler/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_lexer_basic() {
    printf("--- Test: Lexer Basic Tokenization ---\n");
    const char *source = "fn main() { let x = 42; }";
    nova_lexer_t *lexer = nova_lexer_create(source);
    assert(lexer != NULL);

    nova_token_t t;
    
    // fn
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_KEYWORD_FN);
    
    // main
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_IDENT);
    assert(t.length == 4);
    assert(strncmp(t.start, "main", 4) == 0);
    
    // (
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_LPAREN);
    
    // )
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_RPAREN);
    
    // {
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_LBRACE);
    
    // let
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_KEYWORD_LET);
    
    // x
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_IDENT);
    
    // =
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_EQ);
    
    // 42
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_LIT_INT);
    
    // ;
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_SEMICOLON);
    
    // }
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_RBRACE);
    
    // EOF
    t = nova_lexer_next(lexer);
    assert(t.type == TOKEN_EOF);
    
    nova_lexer_destroy(lexer);
    printf("✓ Lexer basic tokenization successful\n");
}

int main(void) {
    printf("=== Running Lexer Unit Tests ===\n");
    test_lexer_basic();
    printf("=== All Lexer Tests Passed! ===\n");
    return 0;
}
