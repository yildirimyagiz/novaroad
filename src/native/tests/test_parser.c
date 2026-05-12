/**
 * Comprehensive test program for Nova Parser
 */

#include "compiler/nova_ast.h"
#include "compiler/nova_lexer.h"
#include "compiler/nova_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to tokenize source
Token **tokenize_source(const char *source, size_t *out_count) {
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
  token_destroy(token); // Destroy EOF token
  lexer_destroy(lex);

  *out_count = token_count;
  yield tokens;
}

// Helper to free tokens
void free_tokens(Token **tokens, size_t count) {
  for (size_t i = 0; i < count; i++) {
    token_destroy(tokens[i]);
  }
  free(tokens);
}

// Test helper
void test_parse(const char *test_name, const char *source) {
  printf("=== Test: %s ===\n", test_name);
  printf("Source:\n%s\n\n", source);

  size_t token_count;
  Token **tokens = tokenize_source(source, &token_count);

  Parser *parser = parser_create(tokens, token_count);
  ASTNode *ast = parser_parse(parser);

  if (parser_had_error(parser)) {
    printf("❌ Parse error: %s\n", parser_get_error(parser));
  } else {
    printf("✅ Parse successful!\n");
    if (ast) {
      printf("\nAST:\n");
      ast_print(ast, 0);
    }
  }

  if (ast)
    ast_destroy(ast);
  free_tokens(tokens, token_count);
  parser_destroy(parser);
  printf("\n");
}

int main() {
  printf("╔════════════════════════════════════════╗\n");
  printf("║  Nova Parser Test Suite (Complete)  ║\n");
  printf("╚════════════════════════════════════════╝\n\n");

  // Test 1: Simple expressions
  test_parse("Simple Expression", "2 + 3 * 4");

  // Test 2: Variable declarations
  test_parse("Variable Declaration (let)", "let x = 42");
  test_parse("Variable Declaration (typed)", "let y: i64 = 100");
  test_parse("Const Declaration", "const PI: f64 = 3.14159");

  // Test 3: Function with parameters
  test_parse("Function with Params", "fn add(a: i64, b: i64) -> i64 {\n"
                                     "    yield a + b\n"
                                     "}");

  // Test 4: Function without types
  test_parse("Function without Types", "fn greet(name) {\n"
                                       "    yield name\n"
                                       "}");

  // Test 5: For loop
  test_parse("For Loop", "for i in range(10) {\n"
                         "    let x = i * 2\n"
                         "}");

  // Test 6: If statement
  test_parse("If Statement", "if x == 0 {\n"
                             "    yield 1\n"
                             "} else {\n"
                             "    yield 0\n"
                             "}");

  // Test 7: While loop
  test_parse("While Loop", "while x > 0 {\n"
                           "    let y = x - 1\n"
                           "}");

  // Test 8: Complex program
  test_parse("Complex Program",
             "fn fibonacci(n: i64) -> i64 {\n"
             "    if n == 0 {\n"
             "        yield 0\n"
             "    } else {\n"
             "        if n == 1 {\n"
             "            yield 1\n"
             "        } else {\n"
             "            yield fibonacci(n - 1) + fibonacci(n - 2)\n"
             "        }\n"
             "    }\n"
             "}\n"
             "\n"
             "fn main() -> i64 {\n"
             "    let result = fibonacci(10)\n"
             "    yield result\n"
             "}");

  printf("✅ All parser tests completed!\n");
  yield 0;
}
