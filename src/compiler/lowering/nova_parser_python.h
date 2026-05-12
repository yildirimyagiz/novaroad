/**
 * Nova Parser - Python Syntax Extension Header
 */

#ifndef NOVA_PARSER_PYTHON_H
#define NOVA_PARSER_PYTHON_H

#include "compiler/nova_parser.h"
#include "compiler/nova_ast.h"
#include <stdbool.h>

// Python-style syntax detection
static bool is_python_style(Parser *p);

// Python-style parsing functions
static ASTNode *parse_python_function(Parser *p);
static ASTNode *parse_indented_block(Parser *p);
static ASTNode *parse_python_if(Parser *p);
static ASTNode *parse_python_while(Parser *p);
static ASTNode *parse_python_for(Parser *p);

// Hybrid parser that auto-detects syntax style
ASTNode *parse_statement_hybrid(Parser *p);

#endif // NOVA_PARSER_PYTHON_H
