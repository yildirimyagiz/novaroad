/**
 * @file parser.h
 * @brief Syntax parser for Nova language
 */

#ifndef NOVA_COMPILER_PARSER_H
#define NOVA_COMPILER_PARSER_H

#include "compiler/lexer.h"
#include "compiler/ast.h"

/**
 * Opaque parser structure
 */
typedef struct nova_parser nova_parser_t;

/**
 * Create a new parser
 * @param lexer Lexer instance to read tokens from
 * @return New parser instance or NULL on error
 */
nova_parser_t *nova_parser_create(nova_lexer_t *lexer);

/**
 * Parse source code into AST
 * @param parser Parser instance
 * @return Root AST node or NULL on error
 */
nova_ast_node_t *nova_parser_parse(nova_parser_t *parser);

/**
 * Get last parse error message
 * @param parser Parser instance
 * @return Error message or NULL if no error
 */
const char *nova_parser_get_error(nova_parser_t *parser);

/**
 * Destroy parser and free resources
 * @param parser Parser instance
 */
void nova_parser_destroy(nova_parser_t *parser);

#endif /* NOVA_COMPILER_PARSER_H */
