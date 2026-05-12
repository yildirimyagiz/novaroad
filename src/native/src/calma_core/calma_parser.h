#ifndef CALMA_PARSER_H
#define CALMA_PARSER_H

#include "calma_lexer.c" // Simplified for monolithic native build

typedef enum {
  NODE_PROGRAM,
  NODE_VAR_DECL,
  NODE_FN_DEF,
  NODE_IF_STMT,
  NODE_EXPR_STMT,
  NODE_BINARY_EXPR,
  NODE_LITERAL_EXPR,
  NODE_IDENTIFIER_EXPR,
  NODE_CALL_EXPR
} NodeType;

struct Node; // Forward decl

typedef struct {
  char *name;
  struct Node *value;
} VarDecl;

typedef struct {
  char *name;
  struct Node *body; // Simplified
} FnDef;

typedef struct {
  struct Node *left;
  char *op;
  struct Node *right;
} BinaryExpr;

typedef struct Node {
  NodeType type;
  union {
    VarDecl var_decl;
    FnDef fn_def;
    BinaryExpr binary;
    char *literal_val;
    char *id_name;
  } as;
  struct Node *next; // For linked list of statements
} Node;

Node *parse_program(Lexer *l);
Node *parse_statement(Lexer *l);
Node *parse_expression(Lexer *l);

#endif
