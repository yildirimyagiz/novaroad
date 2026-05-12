/**
 * @file nova_bootstrap_lexer.c
 * @brief Bootstrap lexer - Simple C implementation for Nova
 */

#include "nova_bootstrap_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Token and Lexer structs defined in header

Lexer *lexer_create(const char *source) {
    Lexer *lex = malloc(sizeof(Lexer));
    lex->source = source;
    lex->current = source;
    lex->line = 1;
    lex->column = 1;
    return lex;
}

void lexer_destroy(Lexer *lex) {
    free(lex);
}

static bool is_at_end(Lexer *lex) {
    return *lex->current == '\0';
}

static char advance(Lexer *lex) {
    lex->column++;
    return *lex->current++;
}

static char peek(Lexer *lex) {
    return *lex->current;
}

static char peek_next(Lexer *lex) {
    if (is_at_end(lex)) return '\0';
    return lex->current[1];
}

static void skip_whitespace(Lexer *lex) {
    while (true) {
        char c = peek(lex);
        if (c == ' ' || c == '\r' || c == '\t') {
            advance(lex);
        } else if (c == '\n') {
            lex->line++;
            lex->column = 0;
            advance(lex);
        } else if (c == '/' && peek_next(lex) == '/') {
            while (peek(lex) != '\n' && !is_at_end(lex)) advance(lex);
        } else {
            break;
        }
    }
}

static Token make_token(Lexer *lex, TokenType type, const char *start, int length) {
    Token tok;
    tok.type = type;
    tok.start = start;
    tok.length = length;
    tok.line = lex->line;
    tok.column = lex->column - length;
    return tok;
}

static Token identifier(Lexer *lex) {
    const char *start = lex->current - 1;
    while (isalnum(peek(lex)) || peek(lex) == '_') {
        advance(lex);
    }
    
    int len = (int)(lex->current - start);
    
    // Check Nova keywords (sorted by length for optimization)
    // 2 chars
    if (len == 2) {
        if (memcmp(start, "fn", 2) == 0) return make_token(lex, TOK_FN, start, len);
        if (memcmp(start, "as", 2) == 0) return make_token(lex, TOK_AS, start, len);
        if (memcmp(start, "in", 2) == 0) return make_token(lex, TOK_IN, start, len);
    }
    // 3 chars
    else if (len == 3) {
        if (memcmp(start, "let", 3) == 0) return make_token(lex, TOK_LET, start, len);
        if (memcmp(start, "var", 3) == 0) return make_token(lex, TOK_VAR, start, len);
        if (memcmp(start, "use", 3) == 0) return make_token(lex, TOK_USE, start, len);
        if (memcmp(start, "for", 3) == 0) return make_token(lex, TOK_FOR, start, len);
        if (memcmp(start, "try", 3) == 0) return make_token(lex, TOK_TRY, start, len);
        if (memcmp(start, "tag", 3) == 0) return make_token(lex, TOK_TAG, start, len);
    }
    // 4 chars
    else if (len == 4) {
        if (memcmp(start, "else", 4) == 0) return make_token(lex, TOK_ELSE, start, len);
        if (memcmp(start, "data", 4) == 0) return make_token(lex, TOK_DATA, start, len);
        if (memcmp(start, "each", 4) == 0) return make_token(lex, TOK_EACH, start, len);
        if (memcmp(start, "loop", 4) == 0) return make_token(lex, TOK_LOOP, start, len);
        if (memcmp(start, "next", 4) == 0) return make_token(lex, TOK_NEXT, start, len);
        if (memcmp(start, "open", 4) == 0) return make_token(lex, TOK_OPEN, start, len);
        if (memcmp(start, "flow", 4) == 0) return make_token(lex, TOK_FLOW, start, len);
        if (memcmp(start, "from", 4) == 0) return make_token(lex, TOK_FROM, start, len);
        if (memcmp(start, "true", 4) == 0) return make_token(lex, TOK_TRUE, start, len);
        if (memcmp(start, "null", 4) == 0) return make_token(lex, TOK_NULL, start, len);
    }
    // 5 chars
    else if (len == 5) {
        if (memcmp(start, "yield", 5) == 0) return make_token(lex, TOK_YIELD, start, len);
        if (memcmp(start, "abort", 5) == 0) return make_token(lex, TOK_ABORT, start, len);
        if (memcmp(start, "check", 5) == 0) return make_token(lex, TOK_CHECK, start, len);
        if (memcmp(start, "match", 5) == 0) return make_token(lex, TOK_MATCH, start, len);
        if (memcmp(start, "while", 5) == 0) return make_token(lex, TOK_WHILE, start, len);
        if (memcmp(start, "cases", 5) == 0) return make_token(lex, TOK_CASES, start, len);
        if (memcmp(start, "rules", 5) == 0) return make_token(lex, TOK_RULES, start, len);
        if (memcmp(start, "skill", 5) == 0) return make_token(lex, TOK_SKILL, start, len);
        if (memcmp(start, "space", 5) == 0) return make_token(lex, TOK_SPACE, start, len);
        if (memcmp(start, "given", 5) == 0) return make_token(lex, TOK_GIVEN, start, len);
        if (memcmp(start, "alias", 5) == 0) return make_token(lex, TOK_ALIAS, start, len);
        if (memcmp(start, "async", 5) == 0) return make_token(lex, TOK_ASYNC, start, len);
        if (memcmp(start, "await", 5) == 0) return make_token(lex, TOK_AWAIT, start, len);
        if (memcmp(start, "spawn", 5) == 0) return make_token(lex, TOK_SPAWN, start, len);
        if (memcmp(start, "catch", 5) == 0) return make_token(lex, TOK_CATCH, start, len);
        if (memcmp(start, "false", 5) == 0) return make_token(lex, TOK_FALSE, start, len);
    }
    // 6 chars
    else if (len == 6) {
        if (memcmp(start, "expose", 6) == 0) return make_token(lex, TOK_EXPOSE, start, len);
        if (memcmp(start, "ensure", 6) == 0) return make_token(lex, TOK_ENSURE, start, len);
        if (memcmp(start, "assert", 6) == 0) return make_token(lex, TOK_ASSERT, start, len);
    }
    // 7 chars
    else if (len == 7) {
        if (memcmp(start, "require", 7) == 0) return make_token(lex, TOK_REQUIRE, start, len);
        if (memcmp(start, "derives", 7) == 0) return make_token(lex, TOK_DERIVES, start, len);
    }
    
    return make_token(lex, TOK_IDENT, start, len);
}

static Token number(Lexer *lex) {
    const char *start = lex->current - 1;
    while (isdigit(peek(lex))) {
        advance(lex);
    }
    if (peek(lex) == '.' && isdigit(peek_next(lex))) {
        advance(lex);
        while (isdigit(peek(lex))) {
            advance(lex);
        }
    }
    return make_token(lex, TOK_NUMBER, start, (int)(lex->current - start));
}

static Token string_tok(Lexer *lex) {
    const char *start = lex->current - 1;
    while (peek(lex) != '"' && !is_at_end(lex)) {
        if (peek(lex) == '\n') lex->line++;
        advance(lex);
    }
    
    if (is_at_end(lex)) {
        return make_token(lex, TOK_ERROR, start, (int)(lex->current - start));
    }
    
    advance(lex); // closing "
    return make_token(lex, TOK_STRING, start, (int)(lex->current - start));
}

Token lexer_next_token(Lexer *lex) {
    skip_whitespace(lex);
    
    if (is_at_end(lex)) {
        return make_token(lex, TOK_EOF, lex->current, 0);
    }
    
    char c = advance(lex);
    
    if (isalpha(c) || c == '_') return identifier(lex);
    if (isdigit(c)) return number(lex);
    
    switch (c) {
        case '(': return make_token(lex, TOK_LPAREN, lex->current - 1, 1);
        case ')': return make_token(lex, TOK_RPAREN, lex->current - 1, 1);
        case '{': return make_token(lex, TOK_LBRACE, lex->current - 1, 1);
        case '}': return make_token(lex, TOK_RBRACE, lex->current - 1, 1);
        case '[': return make_token(lex, TOK_LBRACKET, lex->current - 1, 1);
        case ']': return make_token(lex, TOK_RBRACKET, lex->current - 1, 1);
        case ';': return make_token(lex, TOK_SEMICOLON, lex->current - 1, 1);
        case ',': return make_token(lex, TOK_COMMA, lex->current - 1, 1);
        case '.': return make_token(lex, TOK_DOT, lex->current - 1, 1);
        case '+': return make_token(lex, TOK_PLUS, lex->current - 1, 1);
        case '*': return make_token(lex, TOK_STAR, lex->current - 1, 1);
        case '%': return make_token(lex, TOK_PERCENT, lex->current - 1, 1);
        case '|': return make_token(lex, TOK_PIPE, lex->current - 1, 1);
        case '&': return make_token(lex, TOK_AMPERSAND, lex->current - 1, 1);
        case '"': return string_tok(lex);
        
        case '/':
            if (peek(lex) == '/') {
                // Skip line comment
                while (peek(lex) != '\n' && !is_at_end(lex)) advance(lex);
                return lexer_next_token(lex); // Get next token after comment
            }
            return make_token(lex, TOK_SLASH, lex->current - 1, 1);
            
        case ':':
            if (peek(lex) == ':') {
                advance(lex);
                return make_token(lex, TOK_DOUBLE_COLON, lex->current - 2, 2);
            }
            return make_token(lex, TOK_COLON, lex->current - 1, 1);
            
        case '-':
            if (peek(lex) == '>') {
                advance(lex);
                return make_token(lex, TOK_ARROW, lex->current - 2, 2);
            }
            return make_token(lex, TOK_MINUS, lex->current - 1, 1);
            
        case '=':
            if (peek(lex) == '=') {
                advance(lex);
                return make_token(lex, TOK_EQUAL_EQUAL, lex->current - 2, 2);
            } else if (peek(lex) == '>') {
                advance(lex);
                return make_token(lex, TOK_FAT_ARROW, lex->current - 2, 2);
            }
            return make_token(lex, TOK_EQUAL, lex->current - 1, 1);
            
        case '!':
            if (peek(lex) == '=') {
                advance(lex);
                return make_token(lex, TOK_BANG_EQUAL, lex->current - 2, 2);
            }
            return make_token(lex, TOK_BANG, lex->current - 1, 1);
            
        case '<':
            if (peek(lex) == '=') {
                advance(lex);
                return make_token(lex, TOK_LESS_EQUAL, lex->current - 2, 2);
            }
            return make_token(lex, TOK_LESS, lex->current - 1, 1);
            
        case '>':
            if (peek(lex) == '=') {
                advance(lex);
                return make_token(lex, TOK_GREATER_EQUAL, lex->current - 2, 2);
            }
            return make_token(lex, TOK_GREATER, lex->current - 1, 1);
    }
    
    return make_token(lex, TOK_ERROR, lex->current - 1, 1);
}

const char *token_type_name(TokenType type) {
    switch (type) {
        // Meta
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
        
        // Literals
        case TOK_IDENT: return "IDENT";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_TRUE: return "TRUE";
        case TOK_FALSE: return "FALSE";
        case TOK_NULL: return "NULL";
        
        // Nova Keywords
        case TOK_FN: return "FN";
        case TOK_DATA: return "DATA";
        case TOK_CASES: return "CASES";
        case TOK_RULES: return "RULES";
        case TOK_SKILL: return "SKILL";
        case TOK_FLOW: return "FLOW";
        case TOK_SPACE: return "SPACE";
        case TOK_GIVEN: return "GIVEN";
        case TOK_TAG: return "TAG";
        case TOK_ALIAS: return "ALIAS";
        case TOK_LET: return "LET";
        case TOK_VAR: return "VAR";
        case TOK_OPEN: return "OPEN";
        case TOK_EXPOSE: return "EXPOSE";
        case TOK_DERIVES: return "DERIVES";
        
        // Control Flow
        case TOK_CHECK: return "CHECK";
        case TOK_ELSE: return "ELSE";
        case TOK_MATCH: return "MATCH";
        case TOK_EACH: return "EACH";
        case TOK_FOR: return "FOR";
        case TOK_IN: return "IN";
        case TOK_LOOP: return "LOOP";
        case TOK_WHILE: return "WHILE";
        case TOK_YIELD: return "YIELD";
        case TOK_ABORT: return "ABORT";
        case TOK_NEXT: return "NEXT";
        
        // Module System
        case TOK_USE: return "USE";
        case TOK_FROM: return "FROM";
        case TOK_AS: return "AS";
        
        // Async
        case TOK_ASYNC: return "ASYNC";
        case TOK_AWAIT: return "AWAIT";
        case TOK_SPAWN: return "SPAWN";
        
        // Error Handling
        case TOK_TRY: return "TRY";
        case TOK_CATCH: return "CATCH";
        
        // Contracts
        case TOK_REQUIRE: return "REQUIRE";
        case TOK_ENSURE: return "ENSURE";
        case TOK_ASSERT: return "ASSERT";
        
        // Delimiters
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_LBRACKET: return "LBRACKET";
        case TOK_RBRACKET: return "RBRACKET";
        case TOK_SEMICOLON: return "SEMICOLON";
        case TOK_COLON: return "COLON";
        case TOK_COMMA: return "COMMA";
        case TOK_DOT: return "DOT";
        
        // Operators
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_STAR: return "STAR";
        case TOK_SLASH: return "SLASH";
        case TOK_PERCENT: return "PERCENT";
        case TOK_EQUAL: return "EQUAL";
        case TOK_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TOK_BANG: return "BANG";
        case TOK_BANG_EQUAL: return "BANG_EQUAL";
        case TOK_LESS: return "LESS";
        case TOK_LESS_EQUAL: return "LESS_EQUAL";
        case TOK_GREATER: return "GREATER";
        case TOK_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOK_ARROW: return "ARROW";
        case TOK_FAT_ARROW: return "FAT_ARROW";
        case TOK_PIPE: return "PIPE";
        case TOK_AMPERSAND: return "AMPERSAND";
        case TOK_DOUBLE_COLON: return "DOUBLE_COLON";
        
        default: return "UNKNOWN";
    }
}
