/**
 * @file ast.h
 * @brief Abstract Syntax Tree structures for Nova
 */

#ifndef NOVA_COMPILER_AST_H
#define NOVA_COMPILER_AST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * AST node types
 */
typedef enum {
    AST_LITERAL,
    AST_VARIABLE,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_FUNCTION,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_ASSIGNMENT,
    AST_DECLARATION,
    AST_STRUCT,
    AST_ENUM,
    AST_TRAIT,
    AST_IMPL,
    AST_MATCH,
    AST_IMPORT,
    AST_MODULE,
    AST_PROGRAM
} nova_ast_node_type_t;

/**
 * AST node structure
 */
typedef struct nova_ast_node {
    nova_ast_node_type_t type;
    int line;
    int column;
    
    union {
        struct {
            char *value;
        } literal;
        
        struct {
            char *name;
        } variable;
        
        struct {
            char *op;
            struct nova_ast_node *left;
            struct nova_ast_node *right;
        } binary;
        
        struct {
            char *op;
            struct nova_ast_node *operand;
        } unary;
        
        struct {
            struct nova_ast_node *callee;
            struct nova_ast_node **args;
            size_t arg_count;
        } call;
        
        struct {
            char *name;
            struct nova_ast_node **params;
            size_t param_count;
            struct nova_ast_node *body;
        } function;
        
        struct {
            struct nova_ast_node **statements;
            size_t statement_count;
        } block;
    } data;
} nova_ast_node_t;

/**
 * Create a new AST node
 * @param type Node type
 * @return New AST node or NULL on error
 */
nova_ast_node_t *nova_ast_create_node(nova_ast_node_type_t type);

/**
 * Destroy AST node and all children
 * @param node AST node to destroy
 */
void nova_ast_destroy_node(nova_ast_node_t *node);

/**
 * Print AST for debugging
 * @param node Root node to print
 * @param indent Indentation level
 */
void nova_ast_print(nova_ast_node_t *node, int indent);

#endif /* NOVA_COMPILER_AST_H */
