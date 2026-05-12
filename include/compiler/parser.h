/**
 * @file parser.h
 * @brief Parser for Nova language
 */

#ifndef NOVA_PARSER_H
#define NOVA_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nova_parser nova_parser_t;

/** Create parser from lexer */
nova_parser_t *nova_parser_create(nova_lexer_t *lexer);

/** Parse a single expression */
nova_expr_t *nova_parser_parse_expression(nova_parser_t *parser);

/** Parse a single statement */
nova_stmt_t *nova_parser_parse_statement(nova_parser_t *parser);

/** Parse all statements until EOF */
nova_stmt_t **nova_parser_parse_statements(nova_parser_t *parser, size_t *count);

/** Get last error message */
const char *nova_parser_get_error(nova_parser_t *parser);
nova_token_t nova_parser_get_last_token(nova_parser_t *parser);

/** Check if parser had error */
bool nova_parser_had_error(nova_parser_t *parser);

/** Destroy parser context */
void nova_parser_destroy(nova_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_PARSER_H */