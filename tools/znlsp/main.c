/**
 * @file main.c
 * @brief Nova Language Server - Entry Point
 */

#include "lsp_protocol.h"
#include <stdlib.h>
#include <string.h>

void print_usage(const char *prog)
{
    fprintf(stderr, "Nova Language Server\n");
    fprintf(stderr, "Usage: %s [options]\n\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --stdio    Start LSP server on stdin/stdout (default)\n");
    fprintf(stderr, "  --version  Print version and exit\n");
    fprintf(stderr, "  --help     Show this help\n");
}

int main(int argc, char **argv)
{
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            fprintf(stderr, "Nova Language Server v0.1.0\n");
            return 0;
        } else if (strcmp(argv[i], "--stdio") == 0) {
            // Default mode, continue
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Start LSP server
    return nova_lsp_start();
}
