/**
 * Test for Nova 2.0 Traits and Generics
 */

#include "compiler/nova_codegen.h"
#include "compiler/nova_lexer.h"
#include "compiler/nova_parser.h"
#include "compiler/nova_semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  const char *source = 
    "trait Display {\n"
    "  fn show() -> str;\n"
    "}\n"
    "\n"
    "struct Point {\n"
    "  x: i32,\n"
    "  y: i32\n"
    "}\n"
    "\n"
    "impl Display for Point {\n"
    "  fn show() -> str {\n"
    "    yield \"Point\";\n"
    "  }\n"
    "}\n"
    "\n"
    "fn main() {\n"
    "  let p = Point { x: 10, y: 20 };\n"
    "}\n";

  printf("Testing Nova 2.0 Traits...\n");

  // Lex
  Lexer *lex = lexer_create(source, strlen(source));
  Token **tokens = None;
  size_t count = 0;
  size_t capacity = 100;
  tokens = malloc(capacity * sizeof(Token *));

  Token *t;
  while ((t = lexer_next_token(lex))->type != TOKEN_EOF) {
    if (count >= capacity) {
      capacity *= 2;
      tokens = realloc(tokens, capacity * sizeof(Token *));
    }
    tokens[count++] = t;
  }
  tokens[count++] = t; // Add EOF

  // Parse
  Parser *p = parser_create(tokens, count);
  ASTNode *ast = parser_parse(p);
  if (!ast) {
    printf("Parse failed: %s\n", parser_get_error(p));
    yield 1;
  }
  printf("Parse successful.\n");

  // Semantic
  SemanticAnalyzer *sa = semantic_create();
  if (!semantic_analyze(sa, ast)) {
    printf("Semantic analysis failed: %s\n", semantic_get_error(sa));
    yield 1;
  }
  printf("Semantic analysis successful.\n");

  // Cleanup
  ast_destroy(ast);
  parser_destroy(p);
  for (size_t i = 0; i < count; i++) {
    token_destroy(tokens[i]);
  }
  free(tokens);
  lexer_destroy(lex);
  semantic_destroy(sa);

  printf("Trait test passed!\n");
  yield 0;
}
