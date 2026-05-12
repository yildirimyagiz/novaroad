// Minimal AST for Nova
#ifndef NOVA_MINIMAL_AST_H
#define NOVA_MINIMAL_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct nova_location {
  int line, column;
} nova_location_t;

typedef struct nova_span {
  int file_id;
  size_t start, end, line, col;
} nova_span_t;

typedef struct nova_sourcemgr nova_sourcemgr_t;

typedef enum { DIAG_ERROR, DIAG_WARNING, DIAG_INFO } nova_diag_level_t;

typedef struct nova_diag {
  nova_diag_level_t level;
  nova_span_t span;
  char *message;
} nova_diag_t;

typedef struct nova_diag_collector {
  nova_diag_t **diags;
  size_t count, capacity;
} nova_diag_collector_t;

// AST Node Types
typedef enum {
  AST_LITERAL,
  AST_VARIABLE,
  AST_BINARY_OP,
  AST_UNARY_OP,
  AST_CALL,
  AST_IF,
  AST_WHILE,
  AST_FOR,
  AST_FUNCTION,
  AST_RETURN,
  AST_BLOCK,
  AST_LET,
  AST_ASSIGN,
  AST_MATCH,
  AST_STRUCT,
  AST_ENUM,
  AST_TRAIT,
  AST_IMPL,
} nova_ast_node_type_t;

// Forward declaration
typedef struct nova_ast_node nova_ast_node_t;

// AST Node structure
struct nova_ast_node {
  nova_ast_node_type_t type;
  nova_span_t span;
  struct nova_ast_node *children; // Linked list of children
  struct nova_ast_node *next;     // Next sibling
  union {
    struct {
      void *value;
    } literal;
    struct {
      const char *name;
    } variable;
    struct {
      nova_ast_node_t *callee;
      nova_ast_node_t **args;
      size_t arg_count;
    } call;
    struct {
      nova_ast_node_t **stmts;
      size_t stmt_count;
    } block;
  } data;
};

// Symbol table for type checking
typedef struct symbol {
  const char *name;
  struct nova_type *type;
  struct symbol *next;
  bool is_mutable;
  bool is_global;
  int depth;
  int index;
} symbol_t;

typedef struct symbol_table {
  symbol_t *symbols;
  struct symbol_table *parent;
  size_t next_global_index;
  size_t next_local_index;
  int scope_depth;
  int local_index_stack[64]; // SCOPE_STACK_MAX
} symbol_table_t;

// Semantic analysis context
typedef struct {
  symbol_table_t *current_scope;
  struct nova_type **types;
  size_t type_count;
  symbol_table_t *symbol_table;
  void *ast;
  const char *error_message;
} nova_semantic_t;

// Expression and statement types (forward declarations)
typedef struct nova_expr nova_expr_t;
typedef struct nova_stmt nova_stmt_t;

// === TYPE SYSTEM FOR BORROW CHECKER ===

#include "nova_types.h"



// === FORWARD DECLARATIONS ===
typedef struct nova_expr nova_expr_t;
typedef struct nova_stmt nova_stmt_t;
typedef struct nova_struct_decl nova_struct_decl_t;

// === EXPRESSIONS ===

typedef enum nova_expr_kind {
  EXPR_INT,
  EXPR_FLOAT,
  EXPR_STR,
  EXPR_BOOL,
  EXPR_IDENT,
  EXPR_UNIT_LITERAL, // Unit literal: 5.kg, 9.81.m/s
  EXPR_BINARY,
  EXPR_ASSIGN,
  EXPR_CALL,
  EXPR_HEAP_NEW,
  EXPR_ADDR_OF,
  EXPR_DEREF,
  EXPR_FIELD_ACCESS,
  EXPR_STRUCT_INIT,
  EXPR_STRING_LEN,
  EXPR_STRING_SLICE,
  EXPR_STRING_CONCAT,
  EXPR_CAST,
  EXPR_NAMESPACED_ACCESS,
  EXPR_ARRAY_LIT,
  EXPR_INDEX,
  EXPR_MATCH,
  EXPR_ENUM_VARIANT, // Color::Red or Color::Custom(10, 20, 30)
  EXPR_ASSERT,       // assert(condition, message)
  EXPR_BLOCK,        // { stmt; stmt; tail_expr }
  EXPR_AWAIT,        // await future
  EXPR_UNARY         // -x, !x, ~x
} nova_expr_kind_t;

typedef enum {
  PATTERN_ANY,
  PATTERN_LITERAL,
  PATTERN_IDENT,
  PATTERN_ENUM
} nova_pattern_kind_t;

typedef struct nova_pattern {
  nova_pattern_kind_t kind;
  nova_span_t span;
  union {
    struct nova_expr *literal; // For PATTERN_LITERAL
    char *ident;               // For PATTERN_IDENT
    struct {
      char *name;
      struct nova_pattern **params;
      size_t param_count;
    } variant; // For PATTERN_ENUM
  } data;
} nova_pattern_t;

typedef struct nova_match_arm {
  nova_pattern_t *pattern;
  struct nova_expr *body;
} nova_match_arm_t;

typedef struct nova_contract_stmt {
  struct nova_expr *cond;
  struct nova_expr *message; // Optional
} nova_contract_stmt_t;

typedef struct nova_expr {
  nova_expr_kind_t kind;
  nova_span_t span;
  nova_type_t *type; // inferred type
  union {
    int64_t lit_int;  // for EXPR_INT
    double lit_float; // for EXPR_FLOAT
    char *lit_str;    // for EXPR_STR
    bool lit_bool;    // for EXPR_BOOL
    char *ident;      // for EXPR_IDENT
    struct {
      double value;    // for EXPR_UNIT_LITERAL
      char *unit;      // unit string (e.g., "kg", "m/s")
      void *dimension; // nova_dimension_t pointer
    } unit_literal;
    struct {
      struct nova_expr *left;
      char *op;
      struct nova_expr *right;
    } binary;
    struct {
      struct nova_expr *operand;
      char *op;
    } unary;
    struct {
      char *name;
      struct nova_expr *value;
    } assign;
    struct {
      struct nova_expr *func;
      struct nova_expr **args;
      size_t arg_count;
    } call;
    struct {
      nova_type_t *type;
      struct nova_expr **args;
      size_t arg_count;
    } heap_new;
    struct nova_expr *addr_of; // for EXPR_ADDR_OF
    struct nova_expr *deref;   // for EXPR_DEREF
    struct {
      struct nova_expr *object;
      char *field;
    } field_access;
    struct {
      char *struct_name;
      struct {
        char *name;
        nova_expr_t *value;
      } *fields;
      size_t field_count;
    } struct_init;
    struct {
      struct nova_expr *expr;
      nova_type_t *target_type;
    } cast;
    struct {
      struct nova_expr *object;
      char *member;
    } namespaced_access;
    struct nova_expr *string_len; // for EXPR_STRING_LEN
    struct {
      struct nova_expr *string;
      struct nova_expr *start;
      struct nova_expr *end;
    } string_slice;
    struct {
      struct nova_expr *left;
      struct nova_expr *right;
    } string_concat;
    struct {
      struct nova_expr **elements;
      size_t count;
    } array_lit;
    struct {
      struct nova_expr *object;
      struct nova_expr *index;
    } index;
    struct {
      struct nova_expr *target;
      nova_match_arm_t **arms;
      size_t arm_count;
    } match;
    struct {
      char *enum_name;         // "Color"
      char *variant_name;      // "Red" or "Custom"
      struct nova_expr **args; // Arguments for variants like Custom(10, 20, 30)
      size_t arg_count;
    } enum_variant;
    struct {
      struct nova_expr *cond;
      struct nova_expr *message;
    } assert;
    struct nova_stmt *block;      // for EXPR_BLOCK
    struct nova_expr *await_expr; // for EXPR_AWAIT
  } data;
} nova_expr_t;

// === STATEMENTS ===

typedef enum nova_stmt_kind {
  STMT_EXPR,
  STMT_BLOCK,
  STMT_RETURN,
  STMT_VAR_DECL,
  STMT_HEAP_FREE,
  STMT_STRUCT_DECL,
  STMT_FN,
  STMT_CHECK,
  STMT_WHILE,
  STMT_YIELD,
  STMT_BREAK,
  STMT_CONTINUE,
  STMT_MOD,
  STMT_IMPORT,
  STMT_FOR,
  STMT_ENUM_DECL,
  STMT_EACH,
  STMT_SPAWN,
  STMT_FOREIGN,
  STMT_TRY_CATCH
} nova_stmt_kind_t;

typedef struct nova_stmt {
  nova_stmt_kind_t kind;
  nova_span_t span;
  union {
    nova_expr_t *expr;
    struct {
      struct nova_stmt **statements;
      size_t count;
    } block;
    nova_expr_t *return_expr;
    struct {
      char *name;
      nova_type_t *type;
      nova_expr_t *init;
      bool is_public; /**< true if declared with `pub`/`export` */
    } var_decl;
    nova_expr_t *heap_free;          // for STMT_HEAP_FREE
    nova_struct_decl_t *struct_decl; // for STMT_STRUCT_DECL
    struct {
      char *name;
      struct nova_param **params;
      size_t param_count;
      nova_type_t *return_type;
      struct nova_stmt *body;
      bool is_public; /**< true if declared with `pub`/`export` */
      int target_vpu; /**< 0 for default, or BackendType value */
      nova_contract_stmt_t *
        requires;
      size_t requires_count;
      nova_contract_stmt_t *ensures;
      size_t ensures_count;
    } fn_stmt;
    struct {
      nova_expr_t *condition;
      nova_pattern_t *pattern;
      struct nova_stmt *then_branch;
      struct nova_stmt *else_branch;
    } check_stmt;
    struct {
      nova_expr_t *condition;
      struct nova_stmt *body;
    } while_stmt;
    nova_expr_t *yield_stmt;
    struct {
      char *name;
    } mod_stmt;
    struct {
      char *module_name;
      char **imports;
      size_t import_count;
      char *alias;
    } import_stmt;
    struct {
      char *binding;
      struct nova_expr *start;
      struct nova_expr *end;
      struct nova_stmt *body;
    } for_stmt;
    struct {
      char *name;
      struct {
        char *name;
        struct nova_type **field_types;
        size_t field_count;
      } *variants;
      size_t variant_count;
    } enum_decl;
    struct {
      char *binding;
      struct nova_expr *iterator;
      struct nova_stmt *body;
    } each_stmt;
    struct {
      struct nova_expr *expr; // The spawn expression (usually a call)
    } spawn_stmt;
    struct {
      char *abi;              // "C", "Nova", etc.
      struct nova_stmt *body; // Block containing external declarations
    } foreign_stmt;
    struct {
      struct nova_stmt *try_block;
      struct nova_stmt *catch_block;
      char *exception_name;
    } try_catch_stmt;
    // STMT_BREAK and STMT_CONTINUE have no data
  } data;
} nova_stmt_t;

// API
nova_diag_collector_t *nova_diag_collector_create(void);
void nova_diag_collector_destroy(nova_diag_collector_t *dc);
void nova_diag_error(nova_diag_collector_t *dc, nova_span_t span,
                     const char *message);
void nova_diag_warning(nova_diag_collector_t *dc, nova_span_t span,
                       const char *message);
void nova_diag_print(nova_diag_collector_t *dc, nova_sourcemgr_t *sm);

// Type constructors
nova_type_t *nova_type_void(void);
nova_type_t *nova_type_bool(void);
nova_type_t *nova_type_i32(void);
nova_type_t *nova_type_i64(void);
nova_type_t *nova_type_f32(void);
nova_type_t *nova_type_f64(void);
nova_type_t *nova_type_u8(void);
nova_type_t *nova_type_usize(void);
nova_type_t *nova_type_str(void);
nova_type_t *nova_type_ptr(nova_type_t *pointee); // *T
nova_type_t *nova_type_qty(nova_type_t *base_type,
                           const char *unit_expr); // qty<T, dim>

/// Create a unit literal expression: 5.0.kg, 10.0.km, 9.81.m
nova_expr_t *nova_expr_unit_literal(double value, const char *unit,
                                    nova_location_t loc);
nova_type_t *nova_type_ptr_mut(nova_type_t *pointee); // *mut T
nova_type_t *nova_type_pointer(nova_type_t *pointee); // alias for ptr
nova_type_t *nova_type_array(nova_type_t *element, size_t size);
nova_type_t *nova_type_data(const char *name);

typedef struct nova_param {
  char *name;
  nova_type_t *type;
} nova_param_t;

typedef struct nova_field {
  char *name;
  nova_type_t *type;
} nova_field_t;

// === ADDITIONAL TYPES FOR AST.C ===

typedef struct nova_struct_decl {
  char *name;
  struct {
    char *name;
    nova_type_t *type;
  } *fields;
  size_t field_count;
} nova_struct_decl_t;

typedef enum {
  DECL_FUNCTION,
  DECL_STRUCT,
  DECL_ENUM,
  DECL_VAR,
  DECL_IMPORT
} nova_decl_kind_t;

typedef struct nova_top_level {
  nova_decl_kind_t kind;
  void *data;
} nova_top_level_t;

typedef struct nova_program {
  char *filename;
  nova_top_level_t **declarations;
  size_t declaration_count;
} nova_program_t;

// Function declarations
nova_program_t *nova_program_create(const char *filename);
void nova_program_add_decl(nova_program_t *program, nova_top_level_t *decl);

// Helper functions
nova_location_t nova_location(int line, int column);
nova_span_t nova_span_from_location(nova_location_t loc);

// Expression constructors
nova_expr_t *nova_expr_lit_int(int64_t value, nova_location_t loc);
nova_expr_t *nova_expr_lit_float(double value, nova_location_t loc);
nova_expr_t *nova_expr_lit_str(const char *value, nova_location_t loc);
nova_expr_t *nova_expr_lit_bool(bool value, nova_location_t loc);
nova_expr_t *nova_expr_ident(const char *name, nova_location_t loc);
nova_expr_t *nova_expr_assign(const char *name, nova_expr_t *value,
                              nova_location_t loc);
nova_expr_t *nova_expr_binary(nova_expr_t *left, const char *op,
                              nova_expr_t *right, nova_location_t loc);
nova_expr_t *nova_expr_call(nova_expr_t *callee, nova_expr_t **args,
                            size_t arg_count, nova_location_t loc);
nova_expr_t *nova_expr_heap_new(nova_type_t *type, nova_expr_t **args,
                                size_t arg_count, nova_location_t loc);
nova_expr_t *nova_expr_addr_of(nova_expr_t *expr, nova_location_t loc);
nova_expr_t *nova_expr_deref(nova_expr_t *expr, nova_location_t loc);
nova_expr_t *nova_expr_field_access(nova_expr_t *object, const char *field,
                                    nova_location_t loc);
nova_expr_t *nova_expr_struct_init(const char *struct_name,
                                   nova_location_t loc);
nova_expr_t *nova_expr_string_len(nova_expr_t *string, nova_location_t loc);
nova_expr_t *nova_expr_string_slice(nova_expr_t *string, nova_expr_t *start,
                                    nova_expr_t *end, nova_location_t loc);
nova_expr_t *nova_expr_string_concat(nova_expr_t *left, nova_expr_t *right,
                                     nova_location_t loc);
nova_expr_t *nova_expr_cast(nova_expr_t *expr, nova_type_t *target_type,
                            nova_location_t loc);
nova_expr_t *nova_expr_namespaced_access(nova_expr_t *object,
                                         const char *member,
                                         nova_location_t loc);
nova_expr_t *nova_expr_array_lit(nova_expr_t **elements, size_t count,
                                 nova_location_t loc);
nova_expr_t *nova_expr_index(nova_expr_t *object, nova_expr_t *index,
                             nova_location_t loc);
nova_expr_t *nova_expr_block(nova_stmt_t *block, nova_location_t loc);
nova_expr_t *nova_expr_await(nova_expr_t *expr, nova_location_t loc);
nova_expr_t *nova_expr_unary(nova_expr_t *operand, const char *op,
                             nova_location_t loc);

// Statement constructors
nova_stmt_t *nova_stmt_expr(nova_expr_t *expr, nova_location_t loc);
nova_stmt_t *nova_stmt_heap_free(nova_expr_t *ptr, nova_location_t loc);
nova_stmt_t *nova_stmt_return(nova_expr_t *expr, nova_location_t loc);
nova_stmt_t *nova_stmt_var(const char *name, nova_type_t *type,
                           nova_expr_t *init, nova_location_t loc);
nova_stmt_t *nova_stmt_var_decl(const char *name, nova_type_t *type,
                                nova_expr_t *init, nova_location_t loc);
nova_stmt_t *nova_stmt_block(nova_stmt_t **stmts, size_t count,
                             nova_location_t loc);
nova_stmt_t *nova_stmt_struct_decl(const char *name, nova_location_t loc);
nova_stmt_t *nova_stmt_for(const char *binding, nova_expr_t *start,
                           nova_expr_t *end, nova_stmt_t *body,
                           nova_location_t loc);
nova_stmt_t *nova_stmt_enum_decl(const char *name, nova_location_t loc);
nova_stmt_t *nova_stmt_each(const char *binding, nova_expr_t *iterator,
                            nova_stmt_t *body, nova_location_t loc);
nova_stmt_t *nova_stmt_spawn(nova_expr_t *expr, nova_location_t loc);
nova_stmt_t *nova_stmt_foreign(const char *abi, nova_stmt_t *body,
                               nova_location_t loc);
nova_stmt_t *nova_stmt_try_catch(nova_stmt_t *try_block,
                                 nova_stmt_t *catch_block,
                                 const char *exception_name,
                                 nova_location_t loc);

// Pattern constructors
nova_pattern_t *nova_pattern_any(nova_location_t loc);
nova_pattern_t *nova_pattern_literal(nova_expr_t *literal, nova_location_t loc);
nova_pattern_t *nova_pattern_ident(const char *ident, nova_location_t loc);
nova_pattern_t *nova_pattern_variant(const char *name, nova_pattern_t **params,
                                     size_t param_count, nova_location_t loc);

// Match arm constructor
nova_match_arm_t *nova_match_arm(nova_pattern_t *pattern, nova_expr_t *body);

// Match expression constructor
nova_expr_t *nova_expr_match(nova_expr_t *target, nova_match_arm_t **arms,
                             size_t arm_count, nova_location_t loc);
nova_expr_t *nova_expr_enum_variant(const char *enum_name,
                                    const char *variant_name,
                                    nova_expr_t **args, size_t arg_count,
                                    nova_location_t loc);

// Assert expression constructor
nova_expr_t *nova_expr_assert(nova_expr_t *condition, nova_expr_t *message,
                              nova_location_t loc);

// Memory management
void nova_expr_free(nova_expr_t *expr);
void nova_stmt_free(nova_stmt_t *stmt);
void nova_type_free(nova_type_t *type);

/**
 * nova_type_clone: Deep-copy a type node.
 * The caller owns the returned type and must call nova_type_free() on it.
 */
nova_type_t *nova_type_clone(const nova_type_t *type);
void nova_pattern_free(nova_pattern_t *pattern);
void nova_match_arm_free(nova_match_arm_t *arm);

#endif
