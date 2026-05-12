/**
 * Test program for Nova C Lexer
 */

#include "compiler/nova_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Test basic tokenization
void test_basic_tokens() {
  printf("=== Test: Basic Tokens ===\n");

  const char *source = "fn main() {\n    let x = 42\n    yield x + 10\n}";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    token_print(token);
    token_destroy(token);
  }
  token_destroy(token);

  lexer_destroy(lex);
  printf("\n");
}

// Test string literals
void test_strings() {
  printf("=== Test: String Literals ===\n");

  const char *source = "\"Hello, World!\" 'test' \"escaped\\nstring\"";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    token_print(token);
    token_destroy(token);
  }
  token_destroy(token);

  lexer_destroy(lex);
  printf("\n");
}

// Test numbers
void test_numbers() {
  printf("=== Test: Numbers ===\n");

  const char *source = "42 3.14159 1e10 2.5e-3 0xFF";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    token_print(token);
    token_destroy(token);
  }
  token_destroy(token);

  lexer_destroy(lex);
  printf("\n");
}

// Test operators
void test_operators() {
  printf("=== Test: Operators ===\n");

  const char *source = "+ - * / == != <= >= && || -> =>";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    token_print(token);
    token_destroy(token);
  }
  token_destroy(token);

  lexer_destroy(lex);
  printf("\n");
}

// Test comments
void test_comments() {
  printf("=== Test: Comments ===\n");

  const char *source = "// Line comment\n"
                       "let x = 42 // Another comment\n"
                       "/* Block\n"
                       "   comment */\n"
                       "let y = 10";

  Lexer *lex = lexer_create(source, strlen(source));

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    token_print(token);
    token_destroy(token);
  }
  token_destroy(token);

  lexer_destroy(lex);
  printf("\n");
}

// Performance benchmark
void benchmark_lexer() {
  printf("=== Performance Benchmark ===\n");

  // Generate large source code (1 million tokens)
  const int num_lines = 100000;
  size_t source_size = num_lines * 50; // ~50 chars per line
  char *source = (char *)malloc(source_size);
  char *ptr = source;

  for (int i = 0; i < num_lines; i++) {
    ptr += sprintf(ptr, "fn test_%d(x: i64) -> i64 { yield x + %d }\n", i, i);
  }

  size_t actual_size = ptr - source;
  printf("Source size: %zu bytes\n", actual_size);

  // Benchmark
  clock_t start = clock();

  Lexer *lex = lexer_create(source, actual_size);
  size_t token_count = 0;

  Token *token;
  while ((token = lexer_next_token(lex))->type != TOKEN_EOF) {
    token_count++;
    token_destroy(token);
  }
  token_destroy(token);

  clock_t end = clock();
  double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Tokens: %zu\n", token_count);
  printf("Time: %.3f seconds\n", elapsed);
  printf("Throughput: %.0f tokens/sec\n", token_count / elapsed);
  printf("Throughput: %.0f lines/sec\n", num_lines / elapsed);

  lexer_destroy(lex);
  free(source);
  printf("\n");
}

int main() {
  printf("╔════════════════════════════════════════╗\n");
  printf("║  Nova C Lexer Test Suite            ║\n");
  printf("╚════════════════════════════════════════╝\n\n");

  test_basic_tokens();
  test_strings();
  test_numbers();
  test_operators();
  test_comments();
  benchmark_lexer();

  printf("✅ All tests completed!\n");
  yield 0;
}
