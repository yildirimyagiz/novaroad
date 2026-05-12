/**
 * Nova Error Recovery System
 * Robust error recovery for parser to continue after syntax errors
 */

#ifndef NOVA_ERROR_RECOVERY_H
#define NOVA_ERROR_RECOVERY_H

#include "../compiler/nova_ast.h"
#include "../compiler/nova_lexer.h"
#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// RECOVERY STRATEGIES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  RECOVERY_PANIC_MODE,        // Skip tokens until sync point
  RECOVERY_PHRASE_LEVEL,      // Local corrections (insert/delete token)
  RECOVERY_ERROR_PRODUCTION,  // Use error productions in grammar
  RECOVERY_GLOBAL_CORRECTION, // Minimal distance correction
} RecoveryStrategy;

typedef enum {
  SYNC_STATEMENT,   // Sync to statement boundary (;, }, etc.)
  SYNC_EXPRESSION,  // Sync to expression boundary
  SYNC_DECLARATION, // Sync to declaration keyword
  SYNC_BLOCK,       // Sync to block boundary
} SyncPoint;

// ═══════════════════════════════════════════════════════════════════════════
// ERROR RECOVERY CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  RecoveryStrategy strategy;
  
  // Panic mode state
  bool in_panic_mode;
  SyncPoint sync_point;
  
  // Error tracking
  size_t error_count;
  size_t max_errors;
  
  // Recovery statistics
  size_t successful_recoveries;
  size_t failed_recoveries;
  
  // Synchronization tokens
  TokenType *sync_tokens;
  size_t sync_token_count;
} ErrorRecoveryContext;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

// Context management
ErrorRecoveryContext *error_recovery_create(RecoveryStrategy strategy);
void error_recovery_destroy(ErrorRecoveryContext *ctx);
void error_recovery_reset(ErrorRecoveryContext *ctx);

// Configuration
void error_recovery_set_max_errors(ErrorRecoveryContext *ctx, size_t max_errors);
void error_recovery_add_sync_token(ErrorRecoveryContext *ctx, TokenType type);
void error_recovery_set_sync_point(ErrorRecoveryContext *ctx, SyncPoint point);

// Error recovery
bool error_recovery_attempt(ErrorRecoveryContext *ctx, Lexer *lexer, 
                             const char *expected);
void error_recovery_synchronize(ErrorRecoveryContext *ctx, Lexer *lexer);
void error_recovery_enter_panic_mode(ErrorRecoveryContext *ctx);
void error_recovery_exit_panic_mode(ErrorRecoveryContext *ctx);

// Specific recovery strategies
bool recover_missing_token(ErrorRecoveryContext *ctx, Lexer *lexer, TokenType expected);
bool recover_unexpected_token(ErrorRecoveryContext *ctx, Lexer *lexer);
bool recover_malformed_expression(ErrorRecoveryContext *ctx, Lexer *lexer);
bool recover_malformed_statement(ErrorRecoveryContext *ctx, Lexer *lexer);

// Query
bool error_recovery_should_continue(const ErrorRecoveryContext *ctx);
bool error_recovery_is_in_panic_mode(const ErrorRecoveryContext *ctx);
size_t error_recovery_get_error_count(const ErrorRecoveryContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// SYNC POINT UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

bool is_sync_token(const ErrorRecoveryContext *ctx, TokenType type);
bool is_statement_boundary(TokenType type);
bool is_declaration_keyword(TokenType type);
bool is_expression_starter(TokenType type);

#endif // NOVA_ERROR_RECOVERY_H
