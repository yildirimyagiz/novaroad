#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * CALMA NATIVE LEXER
 * Zero-dependency, high-speed tokenizer written in C.
 */

typedef enum {
  TOKEN_LET,
  TOKEN_FN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_FOR,
  TOKEN_IDENTIFIER,
  TOKEN_INTEGER,
  TOKEN_STRING,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_ASSIGN,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MUL,
  TOKEN_DIV,
  TOKEN_DOT, // Added TOKEN_DOT
  TOKEN_EOF,
  TOKEN_UNKNOWN
} TokenType;

typedef struct {
  TokenType type;
  char *value;
  int line;
  int col;
} Token;

typedef struct {
  const char *source;
  int pos;
  int line;
  int col;
} Lexer;

Token create_token(TokenType type, const char *val, int line, int col) {
  Token t;
  t.type = type;
  t.value = val ? strdup(val) : NULL;
  t.line = line;
  t.col = col;
  return t;
}

Token next_token(Lexer *l) {
  while (l->source[l->pos]) {
    char c = l->source[l->pos];
    if (isspace(c)) {
      if (c == '\n') {
        l->line++;
        l->col = 1;
      } else
        l->col++;
      l->pos++;
    } else if (c == '/' && l->source[l->pos + 1] == '/') {
      // Skip single line comment
      while (l->source[l->pos] && l->source[l->pos] != '\n')
        l->pos++;
    } else {
      break;
    }
  }

  if (!l->source[l->pos])
    return create_token(TOKEN_EOF, NULL, l->line, l->col);

  char c = l->source[l->pos];
  int start_col = l->col;

  // Strings
  if (c == '"') {
    l->pos++;
    l->col++;
    int start = l->pos;
    while (l->source[l->pos] && l->source[l->pos] != '"') {
      if (l->source[l->pos] == '\n') {
        l->line++;
        l->col = 1;
      } else
        l->col++;
      l->pos++;
    }
    int len = l->pos - start;
    char *buf = malloc(len + 1);
    strncpy(buf, l->source + start, len);
    buf[len] = '\0';
    if (l->source[l->pos] == '"') {
      l->pos++;
      l->col++;
    }
    return create_token(TOKEN_STRING, buf, l->line, start_col);
  }

  // Single character tokens
  if (c == '{') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_LBRACE, "{", l->line, start_col);
  }
  if (c == '}') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_RBRACE, "}", l->line, start_col);
  }
  if (c == '(') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_LPAREN, "(", l->line, start_col);
  }
  if (c == ')') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_RPAREN, ")", l->line, start_col);
  }
  if (c == ':') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_COLON, ":", l->line, start_col);
  }
  if (c == '=') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_ASSIGN, "=", l->line, start_col);
  }
  if (c == '+') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_PLUS, "+", l->line, start_col);
  }
  if (c == '*') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_MUL, "*", l->line, start_col);
  }
  if (c == '-') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_MINUS, "-", l->line, start_col);
  }
  if (c == '/') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_DIV, "/", l->line, start_col);
  }
  if (c == '.') { // Added TOKEN_DOT recognition
    l->pos++;
    l->col++;
    return create_token(TOKEN_DOT, ".", l->line, start_col);
  }
  if (c == ';') {
    l->pos++;
    l->col++;
    return create_token(TOKEN_SEMICOLON, ";", l->line, start_col);
  }

  // Identifiers and Keywords
  if (isalpha(c) || c == '_') {
    int start = l->pos;
    while (isalnum(l->source[l->pos]) || l->source[l->pos] == '_')
      l->pos++;
    int len = l->pos - start;
    char *buf = malloc(len + 1);
    strncpy(buf, l->source + start, len);
    buf[len] = '\0';

    TokenType type = TOKEN_IDENTIFIER;
    if (strcmp(buf, "let") == 0)
      type = TOKEN_LET;
    else if (strcmp(buf, "fn") == 0)
      type = TOKEN_FN;
    else if (strcmp(buf, "for") == 0)
      type = TOKEN_FOR;
    else if (strcmp(buf, "if") == 0)
      type = TOKEN_IF;
    else if (strcmp(buf, "else") == 0)
      type = TOKEN_ELSE;

    Token t = create_token(type, buf, l->line, start_col);
    l->col += len;
    free(buf);
    return t;
  }

  // Integers
  if (isdigit(c)) {
    int start = l->pos;
    while (isdigit(l->source[l->pos]))
      l->pos++;
    int len = l->pos - start;
    char *buf = malloc(len + 1);
    strncpy(buf, l->source + start, len);
    buf[len] = '\0';
    Token t = create_token(TOKEN_INTEGER, buf, l->line, start_col);
    l->col += len;
    free(buf);
    return t;
  }

  l->pos++;
  l->col++;
  return create_token(TOKEN_UNKNOWN, NULL, l->line, start_col);
}

const char *token_type_name(TokenType type) {
  switch (type) {
  case TOKEN_LET:
    return "LET";
  case TOKEN_FN:
    return "FN";
  case TOKEN_IF:
    return "IF";
  case TOKEN_ELSE:
    return "ELSE";
  case TOKEN_FOR:
    return "FOR";
  case TOKEN_IDENTIFIER:
    return "IDENTIFIER";
  case TOKEN_INTEGER:
    return "INTEGER";
  case TOKEN_STRING:
    return "STRING";
  case TOKEN_LBRACE:
    return "LBRACE";
  case TOKEN_RBRACE:
    return "RBRACE";
  case TOKEN_LPAREN:
    return "LPAREN";
  case TOKEN_RPAREN:
    return "RPAREN";
  case TOKEN_COLON:
    return "COLON";
  case TOKEN_SEMICOLON:
    return "SEMICOLON";
  case TOKEN_ASSIGN:
    return "ASSIGN";
  case TOKEN_PLUS:
    return "PLUS";
  case TOKEN_MINUS:
    return "MINUS";
  case TOKEN_MUL:
    return "MUL";
  case TOKEN_DIV:
    return "DIV";
  case TOKEN_DOT: // Added TOKEN_DOT case
    return "DOT";
  case TOKEN_EOF:
    return "EOF";
  default:
    return "UNKNOWN";
  }
}

#ifndef NO_LEXER_MAIN
int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <file.zn>\n", argv[0]);
    return 1;
  }
  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    printf("Could not open file %s\n", argv[1]);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *src = malloc(size + 1);
  fread(src, 1, size, f);
  src[size] = '\0';
  fclose(f);

  Lexer l = {src, 0, 1, 1};
  Token t;
  printf("%-15s | %-20s | %-10s\n", "Type", "Value", "Location");
  printf("----------------------------------------------------------\n");
  do {
    t = next_token(&l);
    printf("%-15s | %-20s | %d:%d\n", token_type_name(t.type),
           t.value ? t.value : "", t.line, t.col);
    if (t.value)
      free(t.value);
  } while (t.type != TOKEN_EOF);

  free(src);
  return 0;
}
#endif
