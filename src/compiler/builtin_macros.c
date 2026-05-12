/**
 * Nova Built-in Macros
 * println!, print!, etc.
 */

#include "compiler/ast.h"
#include "compiler/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Expand println! macro to printf call
 * println!("Hello") → printf("Hello\n")
 */
nova_expr_t* expand_println_macro(nova_parser_t* parser, nova_expr_t** args, size_t arg_count) {
    if (arg_count == 0) {
        // println!() → printf("\n")
        nova_expr_t* newline = malloc(sizeof(nova_expr_t));
        newline->kind = EXPR_STRING;
        newline->data.lit_string = strdup("\n");
        
        nova_expr_t* call = malloc(sizeof(nova_expr_t));
        call->kind = EXPR_CALL;
        call->data.call.func = malloc(sizeof(nova_expr_t));
        call->data.call.func->kind = EXPR_IDENT;
        call->data.call.func->data.ident = strdup("printf");
        call->data.call.args = malloc(sizeof(nova_expr_t*));
        call->data.call.args[0] = newline;
        call->data.call.arg_count = 1;
        
        return call;
    }
    
    // println!("format", args...) → printf("format\n", args...)
    // For now, simple version: just add \n to first arg if it's a string
    nova_expr_t* call = malloc(sizeof(nova_expr_t));
    call->kind = EXPR_CALL;
    call->data.call.func = malloc(sizeof(nova_expr_t));
    call->data.call.func->kind = EXPR_IDENT;
    call->data.call.func->data.ident = strdup("printf");
    call->data.call.args = malloc(sizeof(nova_expr_t*) * arg_count);
    call->data.call.arg_count = arg_count;
    
    for (size_t i = 0; i < arg_count; i++) {
        call->data.call.args[i] = args[i];
    }
    
    return call;
}

/**
 * Expand print! macro to printf call
 * print!("Hello") → printf("Hello")
 */
nova_expr_t* expand_print_macro(nova_parser_t* parser, nova_expr_t** args, size_t arg_count) {
    nova_expr_t* call = malloc(sizeof(nova_expr_t));
    call->kind = EXPR_CALL;
    call->data.call.func = malloc(sizeof(nova_expr_t));
    call->data.call.func->kind = EXPR_IDENT;
    call->data.call.func->data.ident = strdup("printf");
    call->data.call.args = malloc(sizeof(nova_expr_t*) * arg_count);
    call->data.call.arg_count = arg_count;
    
    for (size_t i = 0; i < arg_count; i++) {
        call->data.call.args[i] = args[i];
    }
    
    return call;
}

/**
 * Check if identifier is a macro and expand it
 */
nova_expr_t* try_expand_macro(nova_parser_t* parser, const char* name, nova_expr_t** args, size_t arg_count) {
    if (strcmp(name, "println") == 0) {
        return expand_println_macro(parser, args, arg_count);
    }
    if (strcmp(name, "print") == 0) {
        return expand_print_macro(parser, args, arg_count);
    }
    
    return NULL; // Not a macro
}
