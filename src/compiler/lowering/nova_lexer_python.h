/**
 * Nova Lexer - Python Syntax Extension Header
 */

#ifndef NOVA_LEXER_PYTHON_H
#define NOVA_LEXER_PYTHON_H

#include "compiler/nova_lexer.h"
#include <stdbool.h>
#include <stddef.h>

// Indentation stack management
void indent_stack_init(void);
void indent_stack_push(int level);
int indent_stack_pop(void);
int indent_stack_current(void);
void indent_stack_reset(void);
void indent_stack_cleanup(void);

// Indentation analysis
int count_indentation(const char *line);
bool is_blank_line(const char *line, size_t len);

// Python-style lexing
Token **lex_indentation(const char *source, size_t length, size_t *token_count);
Token **lexer_tokenize_hybrid(Lexer *lex, size_t *token_count);

// Token conversion (Python -> C style)
Token **convert_python_to_c_style(Token **python_tokens, size_t python_count, size_t *c_count);

#endif // NOVA_LEXER_PYTHON_H
