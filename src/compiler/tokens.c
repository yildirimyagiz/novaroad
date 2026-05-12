#include <stdbool.h>

/**
 * @file tokens.c
 * @brief Token utility functions — Nova v10 complete catalogue
 */

#include "compiler/tokens.h"
#include <stdio.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// TOKEN TYPE NAMES — debug display strings
// ══════════════════════════════════════════════════════════════════════════════

static const char *token_type_names[] = {
    [TOKEN_EOF] = "EOF",
    [TOKEN_ERROR] = "ERROR",

    /* ── Identifiers and Literals ───────────────────────────────────────── */
    [TOKEN_IDENT] = "IDENT",
    [TOKEN_LIT_INT] = "LIT_INT",
    [TOKEN_LIT_FLOAT] = "LIT_FLOAT",
    [TOKEN_LIT_STR] = "LIT_STR",
    [TOKEN_LIT_CHAR] = "LIT_CHAR",
    [TOKEN_LIT_TRUE] = "LIT_TRUE",
    [TOKEN_LIT_FALSE] = "LIT_FALSE",
    [TOKEN_LIT_UNIT] = "LIT_UNIT",

    /* ── Legacy tokens ──────────────────────────────────────────────────── */
    [TOKEN_IDENTIFIER] = "IDENTIFIER",
    [TOKEN_NUMBER] = "NUMBER",
    [TOKEN_STRING] = "STRING",
    [TOKEN_CHAR] = "CHAR",

    /* ── Core keywords ──────────────────────────────────────────────────── */
    [TOKEN_KEYWORD_FN] = "KEYWORD_FN",
    [TOKEN_KEYWORD_LET] = "KEYWORD_LET",
    [TOKEN_KEYWORD_CONST] = "KEYWORD_CONST",
    [TOKEN_KEYWORD_VAR] = "KEYWORD_VAR",
    [TOKEN_KEYWORD_IF] = "KEYWORD_IF",
    [TOKEN_KEYWORD_CHECK] = "KEYWORD_CHECK",
    [TOKEN_KEYWORD_ELSE] = "KEYWORD_ELSE",
    [TOKEN_KEYWORD_WHILE] = "KEYWORD_WHILE",
    [TOKEN_KEYWORD_FOR] = "KEYWORD_FOR",
    [TOKEN_KEYWORD_RETURN] = "KEYWORD_RETURN",
    [TOKEN_KEYWORD_BREAK] = "KEYWORD_BREAK",
    [TOKEN_KEYWORD_CONTINUE] = "KEYWORD_CONTINUE",
    [TOKEN_KEYWORD_LOOP] = "KEYWORD_LOOP",

    /* ── Legacy declaration keywords ────────────────────────────────────── */
    [TOKEN_KEYWORD_STRUCT] = "KEYWORD_STRUCT",
    [TOKEN_KEYWORD_ENUM] = "KEYWORD_ENUM",
    [TOKEN_KEYWORD_TRAIT] = "KEYWORD_TRAIT",

    /* ── Nova v10 Intent-First keywords ─────────────────────────────────── */
    [TOKEN_KEYWORD_DATA] = "KEYWORD_DATA",
    [TOKEN_KEYWORD_CASES] = "KEYWORD_CASES",
    [TOKEN_KEYWORD_RULES] = "KEYWORD_RULES",
    [TOKEN_KEYWORD_YIELD] = "KEYWORD_YIELD",
    [TOKEN_KEYWORD_EACH] = "KEYWORD_EACH",
    [TOKEN_KEYWORD_FLOW] = "KEYWORD_FLOW",
    [TOKEN_KEYWORD_SPAWN] = "KEYWORD_SPAWN",
    [TOKEN_KEYWORD_GIVEN] = "KEYWORD_GIVEN",
    [TOKEN_KEYWORD_OPEN] = "KEYWORD_OPEN",
    [TOKEN_KEYWORD_ALIAS] = "KEYWORD_ALIAS",
    [TOKEN_KEYWORD_ON] = "KEYWORD_ON",

    /* ── Module & visibility ────────────────────────────────────────────── */
    [TOKEN_KEYWORD_IMPL] = "KEYWORD_IMPL",
    [TOKEN_KEYWORD_TYPE] = "KEYWORD_TYPE",
    [TOKEN_KEYWORD_IMPORT] = "KEYWORD_IMPORT",
    [TOKEN_KEYWORD_EXPORT] = "KEYWORD_EXPORT",
    [TOKEN_KEYWORD_PUB] = "KEYWORD_PUB",
    [TOKEN_KEYWORD_MOD] = "KEYWORD_MOD",
    [TOKEN_KEYWORD_AS] = "KEYWORD_AS",
    [TOKEN_KEYWORD_USE] = "KEYWORD_USE",
    [TOKEN_KEYWORD_MATCH] = "KEYWORD_MATCH",
    [TOKEN_KEYWORD_IN] = "KEYWORD_IN",
    [TOKEN_KEYWORD_EXPOSE] = "KEYWORD_EXPOSE",
    [TOKEN_KEYWORD_CRATE] = "KEYWORD_CRATE",
    [TOKEN_KEYWORD_SELF] = "KEYWORD_SELF",
    [TOKEN_KEYWORD_BASE] = "KEYWORD_BASE",
    [TOKEN_KEYWORD_SUPER] = "KEYWORD_SUPER",

    /* ── Concurrency ────────────────────────────────────────────────────── */
    [TOKEN_KEYWORD_ASYNC] = "KEYWORD_ASYNC",
    [TOKEN_KEYWORD_AWAIT] = "KEYWORD_AWAIT",
    [TOKEN_KEYWORD_ACTOR] = "KEYWORD_ACTOR",

    /* ── Domain-specific / physics / AI keywords ────────────────────────── */
    [TOKEN_KEYWORD_HEAP] = "KEYWORD_HEAP",
    [TOKEN_KEYWORD_FREE] = "KEYWORD_FREE",
    [TOKEN_KEYWORD_NEW] = "KEYWORD_NEW",
    [TOKEN_KEYWORD_SHAPE] = "KEYWORD_SHAPE",
    [TOKEN_KEYWORD_SKILL] = "KEYWORD_SKILL",
    [TOKEN_KEYWORD_UNIT] = "KEYWORD_UNIT",
    [TOKEN_KEYWORD_QTY] = "KEYWORD_QTY",
    [TOKEN_KEYWORD_DIMS] = "KEYWORD_DIMS",
    [TOKEN_KEYWORD_TENSOR] = "KEYWORD_TENSOR",
    [TOKEN_KEYWORD_KERNEL] = "KEYWORD_KERNEL",
    [TOKEN_KEYWORD_BRING] = "KEYWORD_BRING",
    [TOKEN_KEYWORD_APPLY] = "KEYWORD_APPLY",
    [TOKEN_KEYWORD_DERIVES] = "KEYWORD_DERIVES",
    [TOKEN_KEYWORD_KIND] = "KEYWORD_KIND",
    [TOKEN_KEYWORD_MUT] = "KEYWORD_MUT",
    [TOKEN_KEYWORD_REQUIRE] = "KEYWORD_REQUIRE",
    [TOKEN_KEYWORD_ENSURE] = "KEYWORD_ENSURE",
    [TOKEN_KEYWORD_TRY] = "KEYWORD_TRY",
    [TOKEN_KEYWORD_CATCH] = "KEYWORD_CATCH",

    /* ── Operators ──────────────────────────────────────────────────────── */
    [TOKEN_PLUS] = "PLUS",
    [TOKEN_MINUS] = "MINUS",
    [TOKEN_STAR] = "STAR",
    [TOKEN_STAR_STAR] = "STAR_STAR",
    [TOKEN_SLASH] = "SLASH",
    [TOKEN_PERCENT] = "PERCENT",
    [TOKEN_EQ] = "EQ",
    [TOKEN_EQ_EQ] = "EQ_EQ",
    [TOKEN_BANG] = "BANG",
    [TOKEN_BANG_EQ] = "BANG_EQ",
    [TOKEN_LT] = "LT",
    [TOKEN_LT_EQ] = "LT_EQ",
    [TOKEN_GT] = "GT",
    [TOKEN_GT_EQ] = "GT_EQ",
    [TOKEN_AMPERSAND] = "AMPERSAND",
    [TOKEN_AMPERSAND_AMPERSAND] = "AMPERSAND_AMPERSAND",
    [TOKEN_AMP_AMP] = "AMP_AMP",
    [TOKEN_PIPE] = "PIPE",
    [TOKEN_PIPE_PIPE] = "PIPE_PIPE",
    [TOKEN_CARET] = "CARET",
    [TOKEN_TILDE] = "TILDE",

    /* ── Delimiters ─────────────────────────────────────────────────────── */
    [TOKEN_LPAREN] = "LPAREN",
    [TOKEN_RPAREN] = "RPAREN",
    [TOKEN_LBRACE] = "LBRACE",
    [TOKEN_RBRACE] = "RBRACE",
    [TOKEN_LBRACKET] = "LBRACKET",
    [TOKEN_RBRACKET] = "RBRACKET",
    [TOKEN_SEMICOLON] = "SEMICOLON",
    [TOKEN_COMMA] = "COMMA",
    [TOKEN_DOT] = "DOT",
    [TOKEN_DOT_DOT] = "DOT_DOT",
    [TOKEN_COLON] = "COLON",
    [TOKEN_COLON_COLON] = "COLON_COLON",
    [TOKEN_ARROW] = "ARROW",
    [TOKEN_FAT_ARROW] = "FAT_ARROW",
    [TOKEN_QUESTION] = "QUESTION",
    [TOKEN_HASH] = "HASH",
    [TOKEN_AT] = "AT",
};

// ══════════════════════════════════════════════════════════════════════════════
// TOKEN FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Create a token
Token token_create(nova_token_type_t type, const char *start, int length,
                   int line, int column) {
  Token token;
  token.type = type;
  token.start = start;
  token.length = length;
  token.line = line;
  token.column = column;
  token.literal = value_null(); /* Default to null */
  return token;
}

/// Create an error token
Token token_error(const char *message, int line, int column) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = line;
  token.column = column;
  token.literal = value_null();
  return token;
}

/// Get token type name for debugging
const char *token_type_name(nova_token_type_t type) {
  if (type >= sizeof(token_type_names) / sizeof(token_type_names[0])) {
    return "UNKNOWN_TOKEN";
  }
  return token_type_names[type] ? token_type_names[type] : "UNNAMED_TOKEN";
}

void token_print(Token token) {
  printf("Token{type: %s, lexeme: '", token_type_name(token.type));

  for (int i = 0; i < token.length; i++) {
    printf("%c", token.start[i]);
  }

  printf("', line: %d, col: %d", token.line, token.column);

  if (token.type == TOKEN_LIT_INT || token.type == TOKEN_LIT_FLOAT ||
      token.type == TOKEN_LIT_STR) {
    printf(", literal: ");
    value_print(token.literal);
  }

  printf("}");
}
