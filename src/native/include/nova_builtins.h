/**
 * Nova Built-in Functions Registry
 * Defines all built-in functions known to the compiler
 */

#ifndef NOVA_BUILTINS_H
#define NOVA_BUILTINS_H

#include <stdbool.h>

// Check if a function name is a built-in
bool is_builtin_function(const char* name);

// Built-in function signatures (for type checking)
typedef struct {
    const char* name;
    const char* return_type;
    int param_count;
    const char* param_types[8];  // Max 8 params
    bool variadic;  // Accepts variable arguments
} BuiltinFunction;

// Get built-in function info
const BuiltinFunction* get_builtin_function(const char* name);

#endif // NOVA_BUILTINS_H
