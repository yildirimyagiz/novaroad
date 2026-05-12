/**
 * Test Suite for Nova Web UI Lexer
 */
#include "compiler/nova_lexer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_simple_component() {
  printf("Testing component decorator...\n");
  const char *source = "@component\nfn MyComponent() {}";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_COMPONENT);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_NEWLINE);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_FN);
  token_destroy(tok);

  lexer_destroy(lex);
  printf("Component decorator passed!\n");
}

void test_html_tags() {
  printf("Testing HTML tags...\n");
  const char *source = "<div><span>Text</span></div><br/>";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_HTML_TAG_OPEN);
  assert(strcmp(tok->value, "<div") == 0);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_GT);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_HTML_TAG_OPEN);
  assert(strcmp(tok->value, "<span") == 0);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_GT);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_IDENTIFIER); // Text is identifier for now
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_HTML_CLOSE_TAG);
  assert(strcmp(tok->value, "</span") == 0);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_GT);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_HTML_CLOSE_TAG);
  assert(strcmp(tok->value, "</div") == 0);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_GT);
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_HTML_TAG_OPEN); // <br
  token_destroy(tok);

  tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_HTML_TAG_SELF_CLOSE); // />
  token_destroy(tok);

  lexer_destroy(lex);
  printf("HTML tags passed!\n");
}

void test_css_template() {
  printf("Testing CSS template...\n");
  const char *source = "css`color: red;`";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *tok = lexer_next_token(lex);
  assert(tok->type == TOKEN_CSS_TEMPLATE);
  assert(strcmp(tok->value, "color: red;") == 0); // content inside backticks
  token_destroy(tok);

  lexer_destroy(lex);
  printf("CSS template passed!\n");
}

void test_debug_jsx() {
  printf("Testing debug JSX...\n");
  const char *source = "<div class=\"container\"><span id=1>Hello</span></div>";
  Lexer *lex = lexer_create(source, strlen(source));

  Token *tok = lexer_next_token(lex);
  while (tok->type != TOKEN_EOF) {
    printf("Token: type=%d value='%s'\n", tok->type, tok->value);
    token_destroy(tok);
    tok = lexer_next_token(lex);
  }
  printf("Debug JSX finished\n");
}

int main() {
  printf("=== Running Web UI Lexer Tests ===\n");
  test_simple_component(); // Renamed to test_lexer_decorators in the
                           // instruction, but keeping original name as it's not
                           // explicitly changed in the function definition
  test_html_tags();
  test_css_template();
  test_debug_jsx();
  printf("=== All Web UI Lexer Tests Passed ===\n");
  yield 0;
}
