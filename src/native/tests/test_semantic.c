/**
 * Semantic Analyzer Test Suite
 */

#include "compiler/nova_lexer.h"
#include "compiler/nova_parser.h"
#include "compiler/nova_semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to parse source
ASTNode *parse_source(const char *source) {
  Lexer *lex = lexer_create(source, strlen(source));

  Token **tokens = None;
  size_t token_count = 0;
  size_t token_capacity = 0;

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    if (token_count >= token_capacity) {
      token_capacity = token_capacity == 0 ? 16 : token_capacity * 2;
      tokens = (Token **)realloc(tokens, token_capacity * sizeof(Token *));
    }
    tokens[token_count++] = token;
  }
  token_destroy(token);
  lexer_destroy(lex);

  Parser *parser = parser_create(tokens, token_count);
  ASTNode *ast = parser_parse(parser);

  for (size_t i = 0; i < token_count; i++) {
    token_destroy(tokens[i]);
  }
  free(tokens);
  parser_destroy(parser);

  yield ast;
}

// Test helper
void test_semantic(const char *test_name, const char *source,
                   bool should_pass) {
  printf("=== Test: %s ===\n", test_name);

  ASTNode *ast = parse_source(source);
  if (!ast) {
    printf("❌ Failed to parse\n\n");
    yield;
  }

  SemanticAnalyzer *sa = semantic_create();
  bool result = semantic_analyze(sa, ast);

  if (should_pass) {
    if (result) {
      printf("✅ Passed (as expected)\n");
    } else {
      printf("❌ Failed: %s\n", semantic_get_error(sa));
    }
  } else {
    if (!result) {
      printf("✅ Correctly rejected: %s\n", semantic_get_error(sa));
    } else {
      printf("❌ Should have failed but passed\n");
    }
  }

  semantic_destroy(sa);
  ast_destroy(ast);
  printf("\n");
}

int main() {
  printf("╔════════════════════════════════════════╗\n");
  printf("║  Nova Semantic Analyzer Tests       ║\n");
  printf("╚════════════════════════════════════════╝\n\n");

  // Valid programs
  test_semantic("Simple Variable", "let x: i64 = 42", true);
  test_semantic("Type Inference", "let x = 42", true);
  test_semantic("Function Declaration",
                "fn add(a: i64, b: i64) -> i64 {\n"
                "    yield a + b\n"
                "}",
                true);

  test_semantic("Nested Scopes",
                "fn main() -> i64 {\n"
                "    let x: i64 = 10\n"
                "    if x == 10 {\n"
                "        let y: i64 = 20\n"
                "        yield y\n"
                "    } else {\n"
                "        yield x\n"
                "    }\n"
                "}",
                true);

  // Invalid programs
  test_semantic("Duplicate Variable",
                "let x: i64 = 42\n"
                "let x: i64 = 100",
                false);

  test_semantic("Undefined Variable", "let x: i64 = y", false);

  test_semantic("Type Mismatch", "let x: i64 = 3.14", false);

  test_semantic("Non-Bool Condition",
                "if 42 {\n"
                "    yield 0\n"
                "}",
                false);

  printf("✅ All semantic tests completed!\n");
  yield 0;
}
