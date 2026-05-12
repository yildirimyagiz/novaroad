/**
 * @file main.c
 * @brief Nova Compiler (znc) - Main entry point
 */

#include "../../include/compiler/codegen.h"
#include "../../include/compiler/nova_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name) {
  printf("Nova Compiler (znc) v0.1.0\n\n");
  printf("Usage: %s [options] <input.zn>\n\n", program_name);
  printf("Options:\n");
  printf("  -o <file>       Output file\n");
  printf("  -O<level>       Optimization level (0, 1, 2, 3)\n");
  printf("  --backend <b>   Backend: llvm, jit, wasm, vm, cranelift, metal, "
         "spirv\n");
  printf("  --emit <type>   Emit: ast, ir, asm, obj, exe\n");
  printf("  --help          Show this help\n");
  printf("\nExamples:\n");
  printf("  %s hello.zn\n", program_name);
  printf("  %s -o hello physics.zn --backend llvm\n", program_name);
  printf("  %s --jit test.zn\n", program_name);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  const char *input_file = NULL;
  const char *output_file = "a.out";
  nova_backend_type_t backend = NOVA_BACKEND_LLVM;
  int opt_level = 2;

  /* Parse arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_file = argv[++i];
    } else if (strcmp(argv[i], "--backend") == 0 && i + 1 < argc) {
      const char *b = argv[++i];
      if (strcmp(b, "llvm") == 0)
        backend = NOVA_BACKEND_LLVM;
      else if (strcmp(b, "jit") == 0)
        backend = NOVA_BACKEND_JIT;
      else if (strcmp(b, "wasm") == 0)
        backend = NOVA_BACKEND_WASM;
      else if (strcmp(b, "vm") == 0)
        backend = NOVA_BACKEND_VM;
      else {
        fprintf(stderr, "Unknown backend: %s\n", b);
        return 1;
      }
    } else if (strncmp(argv[i], "-O", 2) == 0) {
      opt_level = argv[i][2] - '0';
    } else if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
      nova_integration_add_search_path(argv[++i]);
    } else if (argv[i][0] != '-') {
      input_file = argv[i];
    }
  }

  if (!input_file) {
    fprintf(stderr, "Error: No input file specified\n");
    print_usage(argv[0]);
    return 1;
  }

  /* Compile */
  printf("╔══════════════════════════════════════════════════════════╗\n");
  printf("║  Nova Compiler (znc) v0.1.0                               ║\n");
  printf("╚══════════════════════════════════════════════════════════╝\n\n");

  int result = nova_compile_zn_file(input_file, output_file, backend);

  if (result == 0) {
    printf("\n✅ Compilation successful!\n");
    printf("Output: %s\n", output_file);
    return 0;
  } else {
    fprintf(stderr, "\n❌ Compilation failed!\n");
    return 1;
  }
}
