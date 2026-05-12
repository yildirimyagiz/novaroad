/**
 * Nova AST (Abstract Syntax Tree) Definitions
 * Represents the parsed structure of Nova programs
 */

#ifndef NOVA_AST_H
#define NOVA_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nova_arena.h"

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Type Type;

// ═══════════════════════════════════════════════════════════════════════════
// AST NODE TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  // Program structure
  AST_PROGRAM,
  AST_IMPORT,
  AST_FUNCTION,
  AST_STRUCT,
  AST_ENUM,
  AST_ENUM_VARIANT,
  AST_TRAIT,
  AST_IMPL,

  // Statements
  AST_VARIABLE_DECL,
  AST_CONST_DECL,
  AST_RETURN,
  AST_IF,
  AST_WHILE,
  AST_FOR,
  AST_MATCH,
  AST_DEFER,
  AST_BREAK,
  AST_CONTINUE,
  AST_BLOCK,
  AST_MATCH_ARM,
  AST_EXPR_STMT,

  // Expressions
  AST_BINARY_OP,
  AST_UNARY_OP,
  AST_CALL,
  AST_INDEX,
  AST_MEMBER,
  AST_ASSIGN,
  AST_LAMBDA,
  AST_ARRAY_LITERAL,
  AST_MAP_LITERAL,
  AST_STRUCT_LITERAL,

  // Web UI
  AST_COMPONENT_DECL,
  AST_JSX_ELEMENT,
  AST_JSX_ATTRIBUTE,
  AST_JSX_TEXT,
  AST_JSX_EXPR,
  AST_CSS_TEMPLATE,

  // Literals
  AST_INTEGER,
  AST_FLOAT,
  AST_STRING,
  AST_BOOLEAN,
  AST_NULL,
  AST_IDENTIFIER,

  // Types
  AST_TYPE,
  AST_ARRAY_TYPE,
  AST_MAP_TYPE,
  AST_FUNCTION_TYPE,
  AST_OPTIONAL_TYPE
} ASTNodeType;

// ═══════════════════════════════════════════════════════════════════════════
// TYPE SYSTEM
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  TYPE_VOID,
  TYPE_BOOL,
  TYPE_I8,
  TYPE_I16,
  TYPE_I32,
  TYPE_I64,
  TYPE_U8,
  TYPE_U16,
  TYPE_U32,
  TYPE_U64,
  TYPE_F32,
  TYPE_F64,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_MAP,
  TYPE_STRUCT,
  TYPE_ENUM,
  TYPE_FUNCTION,
  TYPE_OPTIONAL,
  TYPE_REFERENCE,
  TYPE_PARAMETER, // Generic placeholder (T, U)
  TYPE_UNKNOWN
} TypeKind;

struct Type {
  TypeKind kind;
  char *name;
  bool is_mutable; // For &mut T vs &T
  union {
    struct {
      Type *element_type;
      size_t size;
    } array;
    struct {
      Type *key_type;
      Type *value_type;
    } map;
    struct {
      Type **param_types;
      size_t param_count;
      Type *return_type;
    } function;
    Type *wrapped_type; // For optional or reference
    struct {
      Type **args;
      size_t arg_count;
    } generic; // For List<Int>
  } data;
};

// ═══════════════════════════════════════════════════════════════════════════
// AST NODE STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════

struct ASTNode {
  ASTNodeType type;
  size_t line;
  size_t column;
  Type *resolved_type; // Set during semantic analysis

  union {
    // AST_PROGRAM
    struct {
      ASTNode **statements;
      size_t statement_count;
    } program;

    // AST_FUNCTION
    struct {
      char *name;
      char **type_params; // ["T", "U"]
      size_t type_param_count;
      ASTNode **parameters;
      size_t param_count;
      Type *return_type;
      ASTNode *body;
      ASTNode *requires_cond;
      ASTNode *ensures_cond;
      bool is_verified;
      bool is_async;
      bool is_pub;
      uint32_t wcet_us;
      bool has_heuristic_annotation;
      bool is_ai_generated;
      bool auto_deployed;
      bool human_approved;
      bool has_uncertainty_bounds;
      int cyclomatic_complexity;
      int line_count;
    } function;

    // AST_VARIABLE_DECL
    struct {
      char *name;
      Type *var_type;
      ASTNode *initializer;
      bool is_mutable;
    } var_decl;

    // AST_BINARY_OP
    struct {
      char *op;
      ASTNode *left;
      ASTNode *right;
    } binary_op;

    // AST_UNARY_OP
    struct {
      char *op;
      ASTNode *operand;
    } unary_op;

    // AST_CALL
    struct {
      ASTNode *callee;
      ASTNode **arguments;
      size_t arg_count;
      Type **generic_args;
      size_t generic_arg_count;
    } call;

    // AST_IF
    struct {
      ASTNode *condition;
      ASTNode *then_branch;
      ASTNode *else_branch;
    } if_stmt;

    // AST_WHILE
    struct {
      ASTNode *condition;
      ASTNode *body;
    } while_stmt;

    // AST_FOR
    struct {
      char *iterator;
      ASTNode *iterable;
      ASTNode *body;
    } for_stmt;

    // AST_RETURN
    struct {
      ASTNode *value;
    } return_stmt;

    // AST_EXPR_STMT
    struct {
      ASTNode *expression;
    } expr_stmt;

    // AST_BLOCK
    struct {
      ASTNode **statements;
      size_t statement_count;
    } block;

    // AST_ARRAY_LITERAL
    struct {
      ASTNode **elements;
      size_t element_count;
    } array_literal;

    // AST_MAP_LITERAL
    struct {
      ASTNode **keys;
      ASTNode **values;
      size_t pair_count;
    } map_literal;

    // AST_MEMBER
    struct {
      ASTNode *object;
      char *member;
    } member;

    // AST_INDEX
    struct {
      ASTNode *object;
      ASTNode *index;
    } index;

    // AST_ASSIGN
    struct {
      ASTNode *target;
      ASTNode *value;
    } assign;

    // AST_COMPONENT_DECL
    struct {
      char *name;
      ASTNode **params;
      size_t param_count;
      ASTNode *body;
      Type *return_type;
    } component_decl;

    // AST_JSX_ELEMENT
    struct {
      char *tag_name;
      ASTNode **attributes;
      size_t attribute_count;
      ASTNode **children;
      size_t child_count;
      bool is_self_closing;
    } jsx_element;

    // AST_JSX_ATTRIBUTE
    struct {
      char *name;
      ASTNode *value; // Literal or Expression
    } jsx_attribute;

    // AST_JSX_TEXT
    struct {
      char *content;
    } jsx_text;

    // AST_JSX_EXPR
    struct {
      ASTNode *expression;
    } jsx_expr;

    // AST_STRUCT
    struct {
      char *name;
      char **type_params;
      size_t type_param_count;
      char **field_names;
      Type **field_types;
      size_t field_count;
    } struct_decl;

    // AST_TRAIT
    struct {
      char *name;
      char **type_params;
      size_t type_param_count;
      ASTNode **methods; // Array of AST_FUNCTION (usually without bodies)
      size_t method_count;
    } trait_decl;

    // AST_IMPL
    struct {
      char **type_params;
      size_t type_param_count;
      char *trait_name;
      Type **trait_generic_args;
      size_t trait_generic_arg_count;
      char *target_name; // The type being implemented for (e.g., "User")
      Type **target_generic_args;
      size_t target_generic_arg_count;
      ASTNode **methods;
      size_t method_count;
    } impl_decl;

    // AST_ENUM
    struct {
      char *name;
      ASTNode **variants;
      size_t variant_count;
      bool is_pub;
    } enum_decl;

    // AST_ENUM_VARIANT
    struct {
      char *enum_name;       // For expressions: Enum::Variant
      char *variant_name;    // The variant name
      Type **data_types;     // For declaration: types of fields
      ASTNode **data_values; // For expressions: values of fields
      size_t data_count;
    } enum_variant;

    // AST_MATCH
    struct {
      ASTNode *target;
      ASTNode **arms;
      size_t arm_count;
    } match_expr;

    // AST_MATCH_ARM
    struct {
      ASTNode *pattern;
      ASTNode *body;
    } match_arm;

    // AST_STRUCT_LITERAL
    struct {
      char *name;
      char **field_names;
      ASTNode **field_values;
      size_t field_count;
      Type **generic_args;
      size_t generic_arg_count;
    } struct_literal;

    // AST_CSS_TEMPLATE
    struct {
      char *content;
    } css_template;

    // Literals
    int64_t int_value;
    double float_value;
    char *string_value;
    bool bool_value;
    char *identifier;
  } data;
};

// ═══════════════════════════════════════════════════════════════════════════
// AST API
// ═══════════════════════════════════════════════════════════════════════════

// Node creation
ASTNode *ast_create_node(ASTNodeType type, size_t line, size_t column);
ASTNode *ast_create_integer(int64_t value, size_t line, size_t column);
ASTNode *ast_create_float(double value, size_t line, size_t column);
ASTNode *ast_create_string(const char *value, size_t line, size_t column);
ASTNode *ast_create_identifier(const char *name, size_t line, size_t column);
ASTNode *ast_create_binary_op(const char *op, ASTNode *left, ASTNode *right,
                              size_t line, size_t column);
ASTNode *ast_create_unary_op(const char *op, ASTNode *operand, size_t line,
                             size_t column);
ASTNode *ast_create_call(ASTNode *callee, ASTNode **args, size_t arg_count,
                         Type **generic_args, size_t generic_arg_count,
                         size_t line, size_t column);
ASTNode *ast_create_function(const char *name, ASTNode **params,
                             size_t param_count, Type *return_type,
                             ASTNode *body, size_t line, size_t column);
ASTNode *ast_create_if(ASTNode *condition, ASTNode *then_branch,
                       ASTNode *else_branch, size_t line, size_t column);
ASTNode *ast_create_while(ASTNode *condition, ASTNode *body, size_t line,
                          size_t column);
ASTNode *ast_create_return(ASTNode *value, size_t line, size_t column);
ASTNode *ast_create_struct_literal(const char *name, char **field_names,
                                   ASTNode **field_values, size_t field_count,
                                   Type **generic_args,
                                   size_t generic_arg_count, size_t line,
                                   size_t column);
ASTNode *ast_create_impl(char **type_params, size_t type_param_count,
                         const char *trait_name, Type **trait_generic_args,
                         size_t trait_generic_arg_count,
                         const char *target_name, Type **target_generic_args,
                         size_t target_generic_arg_count, ASTNode **methods,
                         size_t method_count, size_t line, size_t column);
ASTNode *ast_create_block(ASTNode **statements, size_t count, size_t line,
                          size_t column);

// Arena-based node creation (Optimized)
ASTNode *ast_create_node_arena(Arena *arena, ASTNodeType type, size_t line,
                               size_t column);
ASTNode *ast_create_integer_arena(Arena *arena, int64_t value, size_t line,
                                  size_t column);
ASTNode *ast_create_float_arena(Arena *arena, double value, size_t line,
                                size_t column);
ASTNode *ast_create_string_arena(Arena *arena, const char *value, size_t line,
                                 size_t column);
ASTNode *ast_create_identifier_arena(Arena *arena, const char *name,
                                     size_t line, size_t column);
ASTNode *ast_create_binary_op_arena(Arena *arena, const char *op, ASTNode *left,
                                    ASTNode *right, size_t line, size_t column);
ASTNode *ast_create_function_arena(Arena *arena, const char *name,
                                   ASTNode **params, size_t param_count,
                                   Type *return_type, ASTNode *body,
                                   size_t line, size_t column);
ASTNode *ast_create_block_arena(Arena *arena, ASTNode **statements,
                                size_t count, size_t line, size_t column);

// Node destruction
void ast_destroy(ASTNode *node);

// Type utilities
Type *type_create(TypeKind kind);
Type *type_create_array(Type *element_type, size_t size);
Type *type_create_function(Type **param_types, size_t param_count,
                           Type *return_type);
Type *type_create_reference(Type *wrapped_type, bool is_mutable);
Type *type_copy(const Type *type);
void type_destroy(Type *type);
bool type_equals(const Type *a, const Type *b);
const char *type_to_string(const Type *type);

// AST printing (for debugging)
void ast_print(const ASTNode *node, int indent);

#endif // NOVA_AST_H
