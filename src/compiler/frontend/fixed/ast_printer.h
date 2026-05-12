/**
 * @file ast_printer.h
 * @brief AST debug printer
 */

#ifndef NOVA_COMPILER_FRONTEND_AST_PRINTER_H
#define NOVA_COMPILER_FRONTEND_AST_PRINTER_H

#include "compiler/ast.h"

/* Print the legacy nova_ast_node_t tree (parser's raw output). */
void nova_ast_print(nova_ast_node_t *node);

/* Print the typed nova_program_t after semantic analysis. */
void nova_program_print(nova_program_t *program);

#endif /* NOVA_COMPILER_FRONTEND_AST_PRINTER_H */
