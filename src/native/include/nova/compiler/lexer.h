/**
 * @file lexer.h
 * @brief Lexical analyzer for Nova language
 */

#ifndef NOVA_COMPILER_LEXER_H
#define NOVA_COMPILER_LEXER_H

#include "compiler/tokens.h"

/**
 * Opaque lexer structure
 */
typedef struct nova_lexer nova_lexer_t;

/**
 * Create a new lexer
 * @param source Source code string (must remain valid during lexer lifetime)
 * @return New lexer instance or NULL on error
 */
nova_lexer_t *nova_lexer_create(const char *source);

/**
 * Get next token from source
 * @param lexer Lexer instance
 * @return Next token
 */
nova_token_t nova_lexer_next(nova_lexer_t *lexer);

/**
 * Peek at next token without consuming it
 * @param lexer Lexer instance
 * @return Next token (lexer state unchanged)
 */
nova_token_t nova_lexer_peek(nova_lexer_t *lexer);

/**
 * Destroy lexer and free resources
 * @param lexer Lexer instance
 */
void nova_lexer_destroy(nova_lexer_t *lexer);

#endif /* NOVA_COMPILER_LEXER_H */
