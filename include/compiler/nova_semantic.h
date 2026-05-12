/**
 * Nova Semantic Analyzer
 * Type checking, symbol resolution, and semantic validation
 */

#ifndef NOVA_SEMANTIC_H
#define NOVA_SEMANTIC_H

#include "nova_ast.h"
#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// SYMBOL TABLE
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  SYMBOL_VARIABLE,
  SYMBOL_FUNCTION,
  SYMBOL_PARAMETER,
  SYMBOL_TYPE,
  SYMBOL_TRAIT,
  SYMBOL_STRUCT
} SymbolKind;

typedef enum {
  STATE_AVAILABLE,
  STATE_MOVED,
  STATE_BORROWED,
  STATE_MUT_BORROWED
} SymbolState;

typedef struct Symbol {
  char *name;
  SymbolKind kind;
  Type *type;
  bool is_mutable;
  bool is_initialized;
  SymbolState state;
  size_t scope_level;
  ASTNode *decl_node;  // Store declaration node for traits/structs
  struct Symbol *next; // For hash table chaining
  // Move tracking (Rust-style ownership)
  size_t move_location; // Line where value was moved (0 if not moved)
} Symbol;

typedef struct Scope {
  Symbol **symbols; // Hash table of symbols
  size_t capacity;
  size_t count;
  size_t level;
  struct Scope *parent;
} Scope;

// Symbol table operations
Scope *scope_create(Scope *parent, size_t level);
void scope_destroy(Scope *scope);
Symbol *scope_lookup(Scope *scope, const char *name);
Symbol *scope_lookup_current(Scope *scope, const char *name);
bool scope_insert(Scope *scope, Symbol *symbol);

// Symbol operations
Symbol *symbol_create(const char *name, SymbolKind kind, Type *type);
void symbol_destroy(Symbol *symbol);

// ═══════════════════════════════════════════════════════════════════════════
// SEMANTIC ANALYZER
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  Scope *current_scope;
  Scope *global_scope;
  size_t current_level;

  // Error tracking
  bool had_error;
  char *error_message;
  size_t error_line;
  size_t error_column;

  // Current function context (for return type checking)
  Type *current_function_return_type;

  // Global implementation registry
  ASTNode **impl_nodes;
  size_t impl_count;
  size_t impl_capacity;
  
  // Borrow checker
  void *borrow_set; // Opaque pointer to BorrowSet
} SemanticAnalyzer;

// Analyzer lifecycle
SemanticAnalyzer *semantic_create();
void semantic_destroy(SemanticAnalyzer *sa);

// Main analysis entry point
bool semantic_analyze(SemanticAnalyzer *sa, ASTNode *ast);

// Error handling
bool semantic_had_error(const SemanticAnalyzer *sa);
const char *semantic_get_error(const SemanticAnalyzer *sa);
size_t semantic_get_error_line(const SemanticAnalyzer *sa);
size_t semantic_get_error_column(const SemanticAnalyzer *sa);

// Scope management
void semantic_enter_scope(SemanticAnalyzer *sa);
void semantic_exit_scope(SemanticAnalyzer *sa);

// Type checking
bool types_equal(const Type *a, const Type *b);
bool type_is_numeric(const Type *type);
bool type_is_integer(const Type *type);
bool type_is_float(const Type *type);
Type *type_get_common(const Type *a, const Type *b);

#endif // NOVA_SEMANTIC_H
