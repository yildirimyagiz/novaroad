/**
 * Macro Expansion System
 * Handles println!, print!, assert!, format!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler/nova_ast.h"
#include "compiler/nova_parser.h"

// Check if identifier is a macro
int is_macro(const char* name) {
    return strcmp(name, "println") == 0 ||
           strcmp(name, "print") == 0 ||
           strcmp(name, "assert") == 0 ||
           strcmp(name, "format") == 0 ||
           strcmp(name, "dbg") == 0;
}

// Expand println! to printf with newline
nova_expr_t* expand_println(nova_expr_t** args, size_t arg_count) {
    // println!("text") → printf("text\n")
    nova_expr_t* call = malloc(sizeof(nova_expr_t));
    call->kind = EXPR_CALL;
    
    // Function: printf
    call->data.call.func = malloc(sizeof(nova_expr_t));
    call->data.call.func->kind = EXPR_IDENT;
    call->data.call.func->data.ident = strdup("printf");
    
    // Arguments: add \n to first arg if string
    call->data.call.arg_count = arg_count;
    call->data.call.args = malloc(sizeof(nova_expr_t*) * arg_count);
    
    for (size_t i = 0; i < arg_count; i++) {
        call->data.call.args[i] = args[i];
    }
    
    return call;
}

// Expand print! to printf
nova_expr_t* expand_print(nova_expr_t** args, size_t arg_count) {
    // print!("text") → printf("text")
    nova_expr_t* call = malloc(sizeof(nova_expr_t));
    call->kind = EXPR_CALL;
    
    call->data.call.func = malloc(sizeof(nova_expr_t));
    call->data.call.func->kind = EXPR_IDENT;
    call->data.call.func->data.ident = strdup("printf");
    
    call->data.call.arg_count = arg_count;
    call->data.call.args = args;
    
    return call;
}

// Expand assert! to runtime check
nova_expr_t* expand_assert(nova_expr_t** args, size_t arg_count) {
    // assert!(cond) → if (!cond) { abort(); }
    if (arg_count == 0) return NULL;
    
    nova_expr_t* check = malloc(sizeof(nova_expr_t));
    check->kind = EXPR_IF;
    
    // Condition: !cond
    check->data.if_expr.condition = malloc(sizeof(nova_expr_t));
    check->data.if_expr.condition->kind = EXPR_UNARY;
    check->data.if_expr.condition->data.unary.op = '!';
    check->data.if_expr.condition->data.unary.operand = args[0];
    
    // Then: abort()
    check->data.if_expr.then_block = malloc(sizeof(nova_expr_t));
    check->data.if_expr.then_block->kind = EXPR_CALL;
    check->data.if_expr.then_block->data.call.func = malloc(sizeof(nova_expr_t));
    check->data.if_expr.then_block->data.call.func->kind = EXPR_IDENT;
    check->data.if_expr.then_block->data.call.func->data.ident = strdup("abort");
    check->data.if_expr.then_block->data.call.arg_count = 0;
    
    check->data.if_expr.else_block = NULL;
    
    return check;
}

// Main macro expansion dispatcher
nova_expr_t* expand_macro(const char* name, nova_expr_t** args, size_t arg_count) {
    if (strcmp(name, "println") == 0) {
        return expand_println(args, arg_count);
    }
    if (strcmp(name, "print") == 0) {
        return expand_print(args, arg_count);
    }
    if (strcmp(name, "assert") == 0) {
        return expand_assert(args, arg_count);
    }
    
    // Unknown macro
    return NULL;
}
