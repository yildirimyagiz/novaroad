/**
 * Nova Pattern Matching System
 * Advanced pattern matching with exhaustiveness checking
 */

#ifndef NOVA_PATTERN_MATCH_H
#define NOVA_PATTERN_MATCH_H

#include "compiler/nova_types.h"
#include "compiler/nova_ast.h"
#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// PATTERN TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  PATTERN_WILDCARD,      // _
  PATTERN_LITERAL,       // 42, "hello", true
  PATTERN_BINDING,       // x, name
  PATTERN_CONSTRUCTOR,   // Some(x), Ok(val)
  PATTERN_TUPLE,         // (x, y, z)
  PATTERN_RECORD,        // { x, y: z }
  PATTERN_OR,            // x | y
  PATTERN_GUARD,         // p if condition
  PATTERN_SLICE,         // [x, y, ..]
  PATTERN_RANGE,         // 0..10
} PatternKind;

typedef struct Pattern {
  PatternKind kind;
  Type *type;           // Type of matched value
  SourceLocation loc;
  
  union {
    // PATTERN_LITERAL
    struct {
      LiteralValue value;
    } literal;
    
    // PATTERN_BINDING
    struct {
      char *name;
      bool mutable;
      struct Pattern *subpattern;  // Optional sub-pattern (e.g., x @ Some(_))
    } binding;
    
    // PATTERN_CONSTRUCTOR
    struct {
      char *constructor_name;
      struct Pattern **fields;
      size_t field_count;
    } constructor;
    
    // PATTERN_TUPLE
    struct {
      struct Pattern **elements;
      size_t element_count;
    } tuple;
    
    // PATTERN_RECORD
    struct {
      char **field_names;
      struct Pattern **field_patterns;
      size_t field_count;
      bool rest;  // .. for partial match
    } record;
    
    // PATTERN_OR
    struct {
      struct Pattern **alternatives;
      size_t alternative_count;
    } or;
    
    // PATTERN_GUARD
    struct {
      struct Pattern *pattern;
      Expr *condition;
    } guard;
    
    // PATTERN_SLICE
    struct {
      struct Pattern **before_rest;
      size_t before_count;
      struct Pattern *rest;  // Optional rest pattern (..)
      struct Pattern **after_rest;
      size_t after_count;
    } slice;
    
    // PATTERN_RANGE
    struct {
      Expr *start;
      Expr *end;
      bool inclusive;
    } range;
  };
} Pattern;

// ═══════════════════════════════════════════════════════════════════════════
// MATCH ARMS
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  Pattern *pattern;
  Expr *guard;         // Optional guard expression
  Expr *body;          // Expression to evaluate if matched
  SourceLocation loc;
} MatchArm;

typedef struct {
  Expr *scrutinee;     // Expression being matched
  MatchArm **arms;
  size_t arm_count;
  Type *result_type;
  SourceLocation loc;
} MatchExpr;

// ═══════════════════════════════════════════════════════════════════════════
// PATTERN CONSTRUCTION
// ═══════════════════════════════════════════════════════════════════════════

Pattern *pattern_create_wildcard(SourceLocation loc);
Pattern *pattern_create_literal(LiteralValue value, SourceLocation loc);
Pattern *pattern_create_binding(const char *name, bool mutable, SourceLocation loc);
Pattern *pattern_create_constructor(const char *name, Pattern **fields, size_t count, SourceLocation loc);
Pattern *pattern_create_tuple(Pattern **elements, size_t count, SourceLocation loc);
Pattern *pattern_create_or(Pattern **alternatives, size_t count, SourceLocation loc);
Pattern *pattern_create_guard(Pattern *pattern, Expr *condition, SourceLocation loc);

void pattern_destroy(Pattern *pattern);

// ═══════════════════════════════════════════════════════════════════════════
// MATCH EXPRESSION
// ═══════════════════════════════════════════════════════════════════════════

MatchExpr *match_expr_create(Expr *scrutinee, SourceLocation loc);
void match_expr_add_arm(MatchExpr *match, Pattern *pattern, Expr *guard, Expr *body);
void match_expr_destroy(MatchExpr *match);

// ═══════════════════════════════════════════════════════════════════════════
// EXHAUSTIVENESS CHECKING
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  bool is_exhaustive;
  Pattern **missing_patterns;  // Patterns not covered
  size_t missing_count;
  Pattern **unreachable_patterns;  // Unreachable patterns
  size_t unreachable_count;
} ExhaustivenessResult;

/**
 * Check if match expression is exhaustive
 * Returns missing patterns if not exhaustive
 */
ExhaustivenessResult *check_exhaustiveness(MatchExpr *match, Type *scrutinee_type);

/**
 * Check for unreachable patterns
 */
bool check_reachability(MatchExpr *match);

void exhaustiveness_result_destroy(ExhaustivenessResult *result);

// ═══════════════════════════════════════════════════════════════════════════
// PATTERN COMPILATION (Decision Trees)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  DECISION_LEAF,        // Final action
  DECISION_SWITCH,      // Switch on constructor/value
  DECISION_GUARD,       // Guard check
  DECISION_FAIL,        // No match found
} DecisionKind;

typedef struct DecisionTree {
  DecisionKind kind;
  
  union {
    // DECISION_LEAF
    struct {
      size_t arm_index;  // Index of matched arm
      Expr *body;
    } leaf;
    
    // DECISION_SWITCH
    struct {
      Expr *scrutinee;
      char **constructor_names;
      struct DecisionTree **branches;
      size_t branch_count;
      struct DecisionTree *default_branch;
    } switch_;
    
    // DECISION_GUARD
    struct {
      Expr *condition;
      struct DecisionTree *then_branch;
      struct DecisionTree *else_branch;
    } guard;
  };
} DecisionTree;

/**
 * Compile match expression to decision tree
 * Optimizes pattern matching for efficient runtime execution
 */
DecisionTree *compile_match_to_decision_tree(MatchExpr *match);

void decision_tree_destroy(DecisionTree *tree);

// ═══════════════════════════════════════════════════════════════════════════
// PATTERN UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Check if pattern always matches (is irrefutable)
 */
bool pattern_is_irrefutable(Pattern *pattern);

/**
 * Extract all bindings from a pattern
 */
char **pattern_get_bindings(Pattern *pattern, size_t *out_count);

/**
 * Check if two patterns overlap (can match same value)
 */
bool patterns_overlap(Pattern *p1, Pattern *p2);

/**
 * Simplify pattern (optimize)
 */
Pattern *pattern_simplify(Pattern *pattern);

/**
 * Pretty-print pattern for error messages
 */
char *pattern_to_string(Pattern *pattern);

// ═══════════════════════════════════════════════════════════════════════════
// INTEGRATION WITH CODEGEN
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Lower pattern match to IR
 * Generates efficient code using decision trees
 */
IRValue *codegen_match_expr(CodegenContext *ctx, MatchExpr *match);

/**
 * Generate code for pattern binding extraction
 */
void codegen_pattern_bindings(CodegenContext *ctx, Pattern *pattern, IRValue *scrutinee);

#endif /* NOVA_PATTERN_MATCH_H */
