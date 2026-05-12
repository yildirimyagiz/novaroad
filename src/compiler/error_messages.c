/**
 * Enhanced Error Messages
 * Better diagnostics with suggestions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* file;
    int line;
    int column;
    const char* message;
    const char* suggestion;
} NovaError;

// Print error with context
void print_error_with_context(NovaError* err, const char* source_line) {
    fprintf(stderr, "\033[1;31merror\033[0m: %s\n", err->message);
    fprintf(stderr, "  --> %s:%d:%d\n", err->file, err->line, err->column);
    fprintf(stderr, "   |\n");
    fprintf(stderr, "%2d | %s\n", err->line, source_line);
    fprintf(stderr, "   | ");
    
    // Print pointer to error location
    for (int i = 0; i < err->column - 1; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "\033[1;31m^\033[0m\n");
    
    // Print suggestion if available
    if (err->suggestion) {
        fprintf(stderr, "   |\n");
        fprintf(stderr, "   = \033[1;36mhelp\033[0m: %s\n", err->suggestion);
    }
    fprintf(stderr, "\n");
}

// Suggest fixes for common errors
const char* get_suggestion(const char* error_type) {
    if (strstr(error_type, "expected")) {
        return "try adding the missing token";
    }
    if (strstr(error_type, "undeclared")) {
        return "make sure the variable is declared before use";
    }
    if (strstr(error_type, "type mismatch")) {
        return "check the types of your expressions";
    }
    if (strstr(error_type, "missing semicolon")) {
        return "add a semicolon at the end of the statement";
    }
    return NULL;
}

// Format error message with color
void nova_report_error(const char* file, int line, int col, const char* msg) {
    NovaError err = {
        .file = file,
        .line = line,
        .column = col,
        .message = msg,
        .suggestion = get_suggestion(msg)
    };
    
    // Would need source line here in real implementation
    const char* dummy_line = "    let x = ;";
    print_error_with_context(&err, dummy_line);
}
