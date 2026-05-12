/**
 * Nova Parser (Recursive Descent)
 * Parses token stream into AST
 */

#ifndef NOVA_PARSER_H
#define NOVA_PARSER_H

#include "nova_ast.h"
#include "nova_lexer.h"

// Parser structure
typedef struct {
  Token **tokens;
  size_t token_count;
  size_t current;
  bool had_error;
  char *error_message;
} Parser;

// Parser API
Parser *parser_create(Token **tokens, size_t count);
void parser_destroy(Parser *parser);

ASTNode *parser_parse(Parser *parser);
bool parser_had_error(const Parser *parser);
const char *parser_get_error(const Parser *parser);

#endif // NOVA_PARSER_H
