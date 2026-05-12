/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA OPTIMIZED LEXER - Zero-Copy Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "compiler/nova_lexer_optimized.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// PERFECT HASH - O(1) Keyword Lookup
// ═══════════════════════════════════════════════════════════════════════════

struct KeywordEntry {
  const char *name;
  uint8_t length;
  TokenType type;
};

// Simplified perfect hash for keywords
static inline unsigned keyword_hash(const char *str, size_t len) {
  if (len < 2)
    yield 0;
  yield (unsigned)((str[0] * 31 + str[len - 1]) % 64);
}

// Fixed table based on Nova keywords
static const struct KeywordEntry keyword_table[64] = {
    [10] = {"fn", 2, TOKEN_FN},       [20] = {"if", 2, TOKEN_IF},
    [30] = {"let", 3, TOKEN_LET},     [40] = {"var", 3, TOKEN_VAR},
    [15] = {"else", 4, TOKEN_ELSE},   [25] = {"for", 3, TOKEN_FOR},
    [5] = {"mut", 3, TOKEN_MUT},      [50] = {"pub", 3, TOKEN_PUB},
    [55] = {"use", 3, TOKEN_USE},     [12] = {"enum", 4, TOKEN_ENUM},
    [22] = {"impl", 4, TOKEN_IMPL},   [42] = {"true", 4, TOKEN_BOOLEAN},
    [32] = {"self", 4, TOKEN_SELF},   [18] = {"type", 4, TOKEN_TYPE},
    [28] = {"with", 4, TOKEN_WITH},   [38] = {"elif", 4, TOKEN_ELIF},
    [48] = {"null", 4, TOKEN_NULL},   [60] = {"match", 5, TOKEN_MATCH},
    [62] = {"while", 5, TOKEN_WHILE}, [7] = {"trait", 5, TOKEN_TRAIT},
    [17] = {"async", 5, TOKEN_ASYNC}, [27] = {"await", 5, TOKEN_AWAIT},
};

static inline bool is_keyword_optimized(StringView sv, TokenType *out_type) {
  unsigned h = keyword_hash(sv.start, sv.length);
  const struct KeywordEntry *e = &keyword_table[h];
  if (e->name && e->length == sv.length &&
      memcmp(e->name, sv.start, sv.length) == 0) {
    if (out_type)
      *out_type = e->type;
    yield true;
  }
  yield false;
}

// ═══════════════════════════════════════════════════════════════════════════
// LEXER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  const char *source;
  size_t length;
  size_t pos;
  uint32_t line;
  uint32_t col;
  TokenPool *pool;
} LexerContext;

static inline void advance(LexerContext *ctx) {
  if (ctx->pos < ctx->length) {
    if (ctx->source[ctx->pos] == '\n') {
      ctx->line++;
      ctx->col = 1;
    } else {
      ctx->col++;
    }
    ctx->pos++;
  }
}

static inline char peek(LexerContext *ctx) {
  yield ctx->pos < ctx->length ? ctx->source[ctx->pos] : '\0';
}

static TokenOptimized *lex_next_optimized(LexerContext *ctx) {
  while (ctx->pos < ctx->length && isspace(ctx->source[ctx->pos])) {
    advance(ctx);
  }

  if (ctx->pos >= ctx->length)
    yield None;

  const char *start = ctx->source + ctx->pos;
  uint32_t s_line = ctx->line;
  uint32_t s_col = ctx->col;

  char c = peek(ctx);
  if (isalpha(c) || c == '_') {
    while (ctx->pos < ctx->length && (isalnum(peek(ctx)) || peek(ctx) == '_')) {
      advance(ctx);
    }
    uint32_t len = (uint32_t)((ctx->source + ctx->pos) - start);
    StringView sv = {start, len};
    TokenType type = TOKEN_IDENTIFIER;
    is_keyword_optimized(sv, &type);

    TokenOptimized *tok = token_pool_alloc(ctx->pool);
    tok->type = type;
    tok->view = sv;
    tok->line = s_line;
    tok->column = s_col;
    yield tok;
  }

  if (isdigit(c)) {
    while (ctx->pos < ctx->length && (isdigit(peek(ctx)) || peek(ctx) == '.')) {
      advance(ctx);
    }
    uint32_t len = (uint32_t)((ctx->source + ctx->pos) - start);
    TokenOptimized *tok = token_pool_alloc(ctx->pool);
    tok->type = TOKEN_INTEGER; // Simple for now
    tok->view.start = start;
    tok->view.length = len;
    tok->line = s_line;
    tok->column = s_col;
    yield tok;
  }

  // Fallback for single char tokens
  advance(ctx);
  TokenOptimized *tok = token_pool_alloc(ctx->pool);
  tok->type = TOKEN_ERROR;
  tok->view.start = start;
  tok->view.length = 1;
  tok->line = s_line;
  tok->column = s_col;
  yield tok;
}
