/**
 * Stage 4 Bootstrap — Compiler Main Entry Point
 *
 * This is the Stage 4 bootstrap entry that ties together:
 *   - Stage 1 frontend (lexer, parser, AST)
 *   - Stage 2 semantic analysis & type system
 *   - Stage 3 optimization passes
 *   - Stage 4 backend codegen (LLVM, Metal, WASM, JIT)
 *
 * When compiled with all stage libraries, this produces the
 * native Nova compiler binary (novac).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations — actual symbols come from stage libraries */
extern int nova_compile_pipeline(const char *input, const char *output,
                                 int opt_level, const char *backend);

static void print_usage(const char *prog) {
  printf("Nova Compiler (Stage 4 Bootstrap) v0.2.0\n\n");
  printf("Usage: %s [options] <input.zn>\n\n", prog);
  printf("Options:\n");
  printf("  -o <file>       Output file (default: a.out)\n");
  printf("  -O<level>       Optimization level (0-3)\n");
  printf("  --backend <b>   Backend: llvm, jit, wasm, native\n");
  printf("  --help          Show this help\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  const char *input_file = NULL;
  const char *output_file = "a.out";
  const char *backend = "native";
  int opt_level = 2;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_file = argv[++i];
    } else if (strncmp(argv[i], "-O", 2) == 0) {
      opt_level = argv[i][2] - '0';
    } else if (strcmp(argv[i], "--backend") == 0 && i + 1 < argc) {
      backend = argv[++i];
    } else if (argv[i][0] != '-') {
      input_file = argv[i];
    }
  }

  if (!input_file) {
    fprintf(stderr, "Error: No input file specified\n");
    return 1;
  }

  printf("Nova Compiler (Stage 4) — Compiling %s\n", input_file);
  printf("  Backend: %s | Opt: O%d | Output: %s\n", backend, opt_level,
         output_file);

  /* TODO: Wire up the full pipeline once all stage libraries are linked */
  printf("  [STUB] Full native pipeline not yet wired.\n");
  printf("  Use Stage 0 (nova_stage0.py) for compilation.\n");

  return 0;
}
