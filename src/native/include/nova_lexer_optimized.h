/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA OPTIMIZED LEXER - Zero-Copy Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_LEXER_OPTIMIZED_H
#define NOVA_LEXER_OPTIMIZED_H

#include "compiler/nova_lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
// ZERO-COPY STRING VIEW
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  const char *start;
  uint32_t length;
} StringView;

static inline bool sv_equals(StringView sv, const char *str) {
  size_t len = strlen(str);
  return sv.length == (uint32_t)len && memcmp(sv.start, str, len) == 0;
}

static inline bool sv_equals_simd(StringView sv, const char *str, size_t len) {
  if (sv.length != (uint32_t)len)
    return false;

#if defined(__AVX2__)
  if (len >= 32) {
    size_t i = 0;
    for (; i + 32 <= len; i += 32) {
      __m256i va = _mm256_loadu_si256((__m256i *)(sv.start + i));
      __m256i vb = _mm256_loadu_si256((__m256i *)(str + i));
      __m256i cmp = _mm256_cmpeq_epi8(va, vb);
      if ((unsigned int)_mm256_movemask_epi8(cmp) != 0xFFFFFFFF)
        return false;
    }
    return memcmp(sv.start + i, str + i, len - i) == 0;
  }
#elif defined(__SSE2__)
  if (len >= 16) {
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
      __m128i va = _mm_loadu_si128((__m128i *)(sv.start + i));
      __m128i vb = _mm_loadu_si128((__m128i *)(str + i));
      __m128i cmp = _mm_cmpeq_epi8(va, vb);
      if (_mm_movemask_epi8(cmp) != 0xFFFF)
        return false;
    }
    return memcmp(sv.start + i, str + i, len - i) == 0;
  }
#endif
  return memcmp(sv.start, str, len) == 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// TOKEN POOL
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  TokenType type;
  StringView view;
  uint32_t line;
  uint32_t column;
} TokenOptimized;

typedef struct TokenPoolChunk {
  TokenOptimized tokens[1024];
  size_t used;
  struct TokenPoolChunk *next;
} TokenPoolChunk;

typedef struct {
  TokenPoolChunk *chunks;
  TokenPoolChunk *current;
} TokenPool;

static inline TokenPool *token_pool_create() {
  TokenPool *pool = malloc(sizeof(TokenPool));
  pool->chunks = calloc(1, sizeof(TokenPoolChunk));
  pool->current = pool->chunks;
  return pool;
}

static inline TokenOptimized *token_pool_alloc(TokenPool *pool) {
  if (pool->current->used >= 1024) {
    TokenPoolChunk *next = calloc(1, sizeof(TokenPoolChunk));
    pool->current->next = next;
    pool->current = next;
  }
  return &pool->current->tokens[pool->current->used++];
}

static inline void token_pool_destroy(TokenPool *pool) {
  TokenPoolChunk *c = pool->chunks;
  while (c) {
    TokenPoolChunk *n = c->next;
    free(c);
    c = n;
  }
  free(pool);
}

#endif
