#include "nova_common.h"

/**
 * Nova Error Recovery Implementation
 */

#include "nova_error_recovery.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DEFAULT_MAX_ERRORS 10

// ═══════════════════════════════════════════════════════════════════════════
// CONTEXT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

ErrorRecoveryContext *error_recovery_create(RecoveryStrategy strategy) {
  ErrorRecoveryContext *ctx = (ErrorRecoveryContext *)calloc(1, sizeof(ErrorRecoveryContext));
  if (!ctx) return NULL;
  
  ctx->strategy = strategy;
  ctx->max_errors = DEFAULT_MAX_ERRORS;
  ctx->in_panic_mode = false;
  ctx->sync_point = SYNC_STATEMENT;
  
  // Default sync tokens
  ctx->sync_token_count = 5;
  ctx->sync_tokens = (TokenType *)malloc(ctx->sync_token_count * sizeof(TokenType));
  ctx->sync_tokens[0] = TOKEN_SEMICOLON;
  ctx->sync_tokens[1] = TOKEN_RBRACE;
  ctx->sync_tokens[2] = TOKEN_EOF;
  ctx->sync_tokens[3] = TOKEN_FN;
  ctx->sync_tokens[4] = TOKEN_STRUCT;
  
  return ctx;
}

void error_recovery_destroy(ErrorRecoveryContext *ctx) {
  if (!ctx) return;
  free(ctx->sync_tokens);
  free(ctx);
}

void error_recovery_reset(ErrorRecoveryContext *ctx) {
  if (!ctx) return;
  ctx->error_count = 0;
  ctx->successful_recoveries = 0;
  ctx->failed_recoveries = 0;
  ctx->in_panic_mode = false;
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

void error_recovery_set_max_errors(ErrorRecoveryContext *ctx, size_t max_errors) {
  if (ctx) ctx->max_errors = max_errors;
}

void error_recovery_add_sync_token(ErrorRecoveryContext *ctx, TokenType type) {
  if (!ctx) return;
  
  ctx->sync_tokens = (TokenType *)realloc(ctx->sync_tokens,
                                           (ctx->sync_token_count + 1) * sizeof(TokenType));
  ctx->sync_tokens[ctx->sync_token_count++] = type;
}

void error_recovery_set_sync_point(ErrorRecoveryContext *ctx, SyncPoint point) {
  if (ctx) ctx->sync_point = point;
}

// ═══════════════════════════════════════════════════════════════════════════
// SYNC POINT UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

bool is_sync_token(const ErrorRecoveryContext *ctx, TokenType type) {
  if (!ctx) return false;
  
  for (size_t i = 0; i < ctx->sync_token_count; i++) {
    if (ctx->sync_tokens[i] == type) {
      return true;
    }
  }
  
  return false;
}

bool is_statement_boundary(TokenType type) {
  return type == TOKEN_SEMICOLON || 
         type == TOKEN_RBRACE || 
         type == TOKEN_EOF;
}

bool is_declaration_keyword(TokenType type) {
  return type == TOKEN_FN || 
         type == TOKEN_STRUCT || 
         type == TOKEN_ENUM || 
         type == TOKEN_TRAIT || 
         type == TOKEN_IMPL ||
         type == TOKEN_LET ||
         type == TOKEN_CONST;
}

bool is_expression_starter(TokenType type) {
  return type == TOKEN_IDENTIFIER ||
         type == TOKEN_INTEGER ||
         type == TOKEN_FLOAT ||
         type == TOKEN_STRING ||
         type == TOKEN_TRUE ||
         type == TOKEN_FALSE ||
         type == TOKEN_LPAREN ||
         type == TOKEN_LBRACKET ||
         type == TOKEN_IF ||
         type == TOKEN_MATCH;
}

// ═══════════════════════════════════════════════════════════════════════════
// ERROR RECOVERY
// ═══════════════════════════════════════════════════════════════════════════

void error_recovery_enter_panic_mode(ErrorRecoveryContext *ctx) {
  if (ctx) ctx->in_panic_mode = true;
}

void error_recovery_exit_panic_mode(ErrorRecoveryContext *ctx) {
  if (ctx) ctx->in_panic_mode = false;
}

void error_recovery_synchronize(ErrorRecoveryContext *ctx, Lexer *lexer) {
  if (!ctx || !lexer) return;
  
  error_recovery_enter_panic_mode(ctx);
  
  // Skip tokens until we find a synchronization point
  while (lexer_current(lexer).type != TOKEN_EOF) {
    TokenType current = lexer_current(lexer).type;
    
    // Check if we've reached a sync point
    if (is_sync_token(ctx, current)) {
      error_recovery_exit_panic_mode(ctx);
      ctx->successful_recoveries++;
      return;
    }
    
    // Check for statement boundaries
    if (ctx->sync_point == SYNC_STATEMENT && is_statement_boundary(current)) {
      lexer_advance(lexer);
      error_recovery_exit_panic_mode(ctx);
      ctx->successful_recoveries++;
      return;
    }
    
    // Check for declaration keywords
    if (ctx->sync_point == SYNC_DECLARATION && is_declaration_keyword(current)) {
      error_recovery_exit_panic_mode(ctx);
      ctx->successful_recoveries++;
      return;
    }
    
    lexer_advance(lexer);
  }
  
  ctx->failed_recoveries++;
}

bool error_recovery_attempt(ErrorRecoveryContext *ctx, Lexer *lexer, const char *expected) {
  if (!ctx || !lexer) return false;
  
  ctx->error_count++;
  
  fprintf(stderr, "Syntax error at line %zu: expected %s, got %s\n",
          lexer_current(lexer).line,
          expected,
          token_type_to_string(lexer_current(lexer).type));
  
  if (ctx->error_count >= ctx->max_errors) {
    fprintf(stderr, "Too many errors, stopping parse.\n");
    return false;
  }
  
  switch (ctx->strategy) {
    case RECOVERY_PANIC_MODE:
      error_recovery_synchronize(ctx, lexer);
      break;
      
    case RECOVERY_PHRASE_LEVEL:
      // Try to insert or delete a token
      if (recover_missing_token(ctx, lexer, TOKEN_SEMICOLON)) {
        ctx->successful_recoveries++;
      } else {
        error_recovery_synchronize(ctx, lexer);
      }
      break;
      
    case RECOVERY_ERROR_PRODUCTION:
      // Use error productions (requires grammar support)
      error_recovery_synchronize(ctx, lexer);
      break;
      
    case RECOVERY_GLOBAL_CORRECTION:
      // Try minimal distance correction (complex, simplified here)
      error_recovery_synchronize(ctx, lexer);
      break;
  }
  
  return true;
}

bool recover_missing_token(ErrorRecoveryContext *ctx, Lexer *lexer, TokenType expected) {
  if (!ctx || !lexer) return false;
  
  // Simple heuristic: if next token makes sense, assume current was missing
  TokenType next = lexer_peek(lexer).type;
  
  if (expected == TOKEN_SEMICOLON) {
    if (is_statement_boundary(next) || is_declaration_keyword(next)) {
      // Pretend semicolon was there
      fprintf(stderr, "Recovery: inserting missing semicolon\n");
      return true;
    }
  }
  
  return false;
}

bool recover_unexpected_token(ErrorRecoveryContext *ctx, Lexer *lexer) {
  if (!ctx || !lexer) return false;
  
  fprintf(stderr, "Recovery: skipping unexpected token %s\n",
          token_type_to_string(lexer_current(lexer).type));
  
  lexer_advance(lexer);
  ctx->successful_recoveries++;
  return true;
}

bool recover_malformed_expression(ErrorRecoveryContext *ctx, Lexer *lexer) {
  if (!ctx || !lexer) return false;
  
  // Skip until we find a likely expression boundary
  while (lexer_current(lexer).type != TOKEN_SEMICOLON &&
         lexer_current(lexer).type != TOKEN_COMMA &&
         lexer_current(lexer).type != TOKEN_RPAREN &&
         lexer_current(lexer).type != TOKEN_RBRACKET &&
         lexer_current(lexer).type != TOKEN_EOF) {
    lexer_advance(lexer);
  }
  
  ctx->successful_recoveries++;
  return true;
}

bool recover_malformed_statement(ErrorRecoveryContext *ctx, Lexer *lexer) {
  if (!ctx || !lexer) return false;
  
  // Skip until we find statement boundary
  while (!is_statement_boundary(lexer_current(lexer).type)) {
    lexer_advance(lexer);
  }
  
  if (lexer_current(lexer).type == TOKEN_SEMICOLON) {
    lexer_advance(lexer);
  }
  
  ctx->successful_recoveries++;
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// QUERY
// ═══════════════════════════════════════════════════════════════════════════

bool error_recovery_should_continue(const ErrorRecoveryContext *ctx) {
  return ctx && ctx->error_count < ctx->max_errors;
}

bool error_recovery_is_in_panic_mode(const ErrorRecoveryContext *ctx) {
  return ctx && ctx->in_panic_mode;
}

size_t error_recovery_get_error_count(const ErrorRecoveryContext *ctx) {
  return ctx ? ctx->error_count : 0;
}
