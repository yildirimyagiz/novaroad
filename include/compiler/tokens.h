/**
 * @file tokens.h
 * @brief Token definitions for Nova language
 */

#ifndef NOVA_TOKENS_H
#define NOVA_TOKENS_H

#include "../backend/chunk.h" // For Value type
#include "./lexer.h" // For nova_token_type_t

// ══════════════════════════════════════════════════════════════════════════════
// TOKEN STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

typedef struct {
  nova_token_type_t type;
  const char *start; // Points to the first character of the token in the source
  int length;        // Number of characters in the token
  int line;          // Line number where the token appears
  int column;        // Column number
  Value literal;     // For literals: numbers, strings, etc.
} Token;

// ══════════════════════════════════════════════════════════════════════════════
// TOKEN API
// ══════════════════════════════════════════════════════════════════════════════

/// Create a token
Token token_create(nova_token_type_t type, const char *start, int length,
                   int line, int column);

/// Create an error token
Token token_error(const char *message, int line, int column);

/// Get token type name for debugging
const char *token_type_name(nova_token_type_t type);

/// Print token for debugging
void token_print(Token token);

#endif // NOVA_TOKENS_H
