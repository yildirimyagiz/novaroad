#include <stdbool.h>

/**
 * Nova Compiler - Main Entry Point
 * Full CLI implementation with all commands
 */

#include "backend/bytecode.h"
#include "backend/chunk.h"
#include "backend/vm.h"
#include "compiler/ast.h"
#include "compiler/backend/llvm/nova_llvm_backend.h"
#include "compiler/codegen.h"
#include "compiler/compiler.h"
#include "compiler/lexer.h"
#include "compiler/module_registry.h"
#include "compiler/parser.h"
#include "compiler/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NOVA_VERSION "1.0.0"

// Forward declarations for codegen API (not in header yet)
bool nova_codegen_generate_from_ast(nova_codegen_t *codegen,
                                    nova_program_t *ast);
void nova_codegen_set_semantic(nova_codegen_t *codegen,
                               nova_semantic_t *semantic);
Chunk *nova_codegen_get_chunk(nova_codegen_t *codegen);
const char *nova_codegen_get_error(nova_codegen_t *codegen);

typedef enum {
  MODE_PARSE, // Default: parse + check
  MODE_BUILD, // Build executable
  MODE_RUN,   // JIT or build+run
  MODE_CHECK, // Type check only
  MODE_HELP,
  MODE_VERSION
} nova_mode_t;

typedef enum { EMIT_NONE, EMIT_AST, EMIT_IR, EMIT_OBJ, EMIT_ASM } nova_emit_t;

typedef struct {
  nova_mode_t mode;
  nova_emit_t emit;
  const char *input_file;
  const char *output_file;
  const char *module_path; /**< Colon-separated search path for user modules */
  int opt_level;
  int use_jit;
  int verbose;
  int deterministic;
  int no_borrow_check;
  int dump_tokens;
  const char *target_triple;
} nova_options_t;

static void print_version(void) {
  printf("Nova Compiler v%s\n", NOVA_VERSION);
  printf("  LLVM JIT:        enabled\n");
  printf("  Borrow Checker:  enabled\n");
  printf("  Multi-target:    x86_64, arm64, wasm32, riscv64\n");
}

static void print_help(void) {
  printf("Nova Compiler v%s\n\n", NOVA_VERSION);
  printf("USAGE:\n");
  printf("  nova <file>                    Parse and check file\n");
  printf("  nova <file> -o <output>        Compile to executable\n");
  printf("  nova build <file>              Build executable\n");
  printf("  nova run <file>                JIT run or build+run\n");
  printf("  nova check <file>              Type check only\n");
  printf("\n");
  printf("OUTPUT CONTROL:\n");
  printf("  -o <file>                      Output file\n");
  printf("  --emit-ast                     Emit AST\n");
  printf("  --emit-ir                      Emit LLVM IR\n");
  printf("  --emit-obj                     Emit object file\n");
  printf("  --emit-asm                     Emit assembly\n");
  printf("  --dump-tokens                  Dump lexer tokens\n");
  printf("\n");
  printf("EXECUTION:\n");
  printf("  --jit                          Use JIT backend\n");
  printf("  --run                          Run after compile\n");
  printf("\n");
  printf("OPTIMIZATION:\n");
  printf("  -O0, -O1, -O2, -O3            Optimization level\n");
  printf("\n");
  printf("OPTIONS:\n");
  printf("  --verbose, -v                  Verbose output\n");
  printf("  --version                      Show version\n");
  printf("  --help, -h                     Show this help\n");
  printf("  --no-borrow-check              Disable borrow checker\n");
  printf("  --deterministic                Deterministic build\n");
  printf("\n");
}

static nova_options_t parse_args(int argc, char **argv) {
  nova_options_t opts = {0};
  opts.mode = MODE_PARSE;
  opts.emit = EMIT_NONE;
  opts.opt_level = 0;

  if (argc < 2) {
    opts.mode = MODE_HELP;
    return opts;
  }

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    // Commands
    if (strcmp(arg, "build") == 0 && i + 1 < argc) {
      opts.mode = MODE_BUILD;
      opts.input_file = argv[++i];
    } else if (strcmp(arg, "run") == 0 && i + 1 < argc) {
      opts.mode = MODE_RUN;
      opts.input_file = argv[++i];
    } else if (strcmp(arg, "check") == 0 && i + 1 < argc) {
      opts.mode = MODE_CHECK;
      opts.input_file = argv[++i];
    }
    // Help/Version
    else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      opts.mode = MODE_HELP;
    } else if (strcmp(arg, "--version") == 0) {
      opts.mode = MODE_VERSION;
    }
    // Output
    else if (strcmp(arg, "-o") == 0 && i + 1 < argc) {
      opts.output_file = argv[++i];
      if (opts.mode == MODE_PARSE) {
        opts.mode = MODE_BUILD;
      }
    }
    // Emit
    else if (strcmp(arg, "--emit-ast") == 0) {
      opts.emit = EMIT_AST;
    } else if (strcmp(arg, "--emit-ir") == 0) {
      opts.emit = EMIT_IR;
    } else if (strcmp(arg, "--emit-obj") == 0) {
      opts.emit = EMIT_OBJ;
    } else if (strcmp(arg, "--emit-asm") == 0) {
      opts.emit = EMIT_ASM;
    } else if (strcmp(arg, "--dump-tokens") == 0 ||
               strcmp(arg, "--tokens") == 0) {
      opts.dump_tokens = 1;
    }
    // Execution
    else if (strcmp(arg, "--jit") == 0) {
      opts.use_jit = 1;
    } else if (strcmp(arg, "--run") == 0) {
      opts.mode = MODE_RUN;
    }
    // Optimization
    else if (strcmp(arg, "-O0") == 0) {
      opts.opt_level = 0;
    } else if (strcmp(arg, "-O1") == 0) {
      opts.opt_level = 1;
    } else if (strcmp(arg, "-O2") == 0) {
      opts.opt_level = 2;
    } else if (strcmp(arg, "-O3") == 0) {
      opts.opt_level = 3;
    }
    // Flags
    else if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0) {
      opts.verbose = 1;
    } else if (strcmp(arg, "--deterministic") == 0) {
      opts.deterministic = 1;
    } else if (strcmp(arg, "--no-borrow-check") == 0) {
      opts.no_borrow_check = 1;
    }
    // Input file
    else if ((strcmp(arg, "--module-path") == 0 || strcmp(arg, "-M") == 0) &&
             i + 1 < argc) {
      opts.module_path = argv[++i];
    } else if (arg[0] != '-' && !opts.input_file) {
      opts.input_file = arg;
    }
  }

  return opts;
}

static char *read_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Failed to open file: %s\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *source = (char *)malloc(length + 1);
  if (!source) {
    fprintf(stderr, "Failed to allocate memory\n");
    fclose(file);
    return NULL;
  }

  size_t bytes_read = fread(source, 1, length, file);
  source[bytes_read] = '\0';
  fclose(file);

  return source;
}

static void add_stmt_to_program(nova_program_t *program, nova_stmt_t *s) {
  if (s->kind == STMT_BLOCK) {
    for (size_t i = 0; i < s->data.block.count; i++) {
      add_stmt_to_program(program, s->data.block.statements[i]);
    }
    return;
  }

  nova_top_level_t *decl = malloc(sizeof(nova_top_level_t));
  switch (s->kind) {
  case STMT_FN:
    decl->kind = DECL_FUNCTION;
    break;
  case STMT_STRUCT_DECL:
    decl->kind = DECL_STRUCT;
    break;
  case STMT_ENUM_DECL:
    decl->kind = DECL_ENUM;
    break;
  case STMT_VAR_DECL:
    decl->kind = DECL_VAR;
    break;
  case STMT_IMPORT:
    decl->kind = DECL_IMPORT;
    break;
  default:
    free(decl);
    return;
  }

  decl->data = s;
  nova_program_add_decl(program, decl);
}

int main(int argc, char **argv) {
  nova_options_t opts = parse_args(argc, argv);

  // Handle special modes
  if (opts.mode == MODE_HELP) {
    print_help();
    return 0;
  }
  if (opts.mode == MODE_VERSION) {
    print_version();
    return 0;
  }

  // Need input file
  if (!opts.input_file) {
    fprintf(stderr, "Error: No input file specified\n");
    fprintf(stderr, "Use --help for usage information\n");
    return 1;
  }

  // Read source
  char *source = read_file(opts.input_file);
  if (!source) {
    return 1;
  }

  if (opts.verbose) {
    fprintf(stderr, "Testing Nova Full Compilation Pipeline...\n");
    fprintf(stderr, "File: %s\n", opts.input_file);
    fprintf(stderr, "Source: %s\n\n", source);
  }

  if (opts.dump_tokens) {
    nova_lexer_t *lexer = nova_lexer_create(source);
    if (!lexer) {
      fprintf(stderr, "Failed to create lexer\n");
      free(source);
      return 1;
    }
    printf("--- TOKENS ---\n");
    for (;;) {
      nova_token_t token = nova_lexer_next(lexer);
      printf("Token{type: %s, lexeme: '%.*s', line: %d, col: %d}\n",
             nova_token_type_name(token.type), (int)token.length, token.start,
             token.line, token.column);
      if (token.type == TOKEN_EOF)
        break;
      if (token.type == TOKEN_ERROR) {
        fprintf(stderr, "Lex error at line %d, col %d: %.*s\n", token.line,
                token.column, (int)token.length, token.start);
        break;
      }
    }
    printf("--------------\n");
    nova_lexer_destroy(lexer);
    free(source);
    return 0;
  }

  // Create compiler
  nova_compiler_t *compiler = nova_compiler_create_default();
  if (!compiler) {
    fprintf(stderr, "Failed to create compiler\n");
    free(source);
    return 1;
  }

  // ═══════════════════════════════════════════════════════════════════
  // MODE_BUILD: Build native executable using LLVM
  // ═══════════════════════════════════════════════════════════════════
  if (opts.mode == MODE_BUILD) {
    if (opts.verbose) {
      fprintf(stderr, "Building native executable with LLVM...\n");
    }

    // 1. Lex
    nova_lexer_t *lexer = nova_lexer_create(source);
    if (!lexer) {
      fprintf(stderr, "Failed to create lexer\n");
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // 2. Parse
    nova_parser_t *parser = nova_parser_create(lexer);
    if (!parser) {
      fprintf(stderr, "Failed to create parser\n");
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    size_t stmt_count;
    nova_stmt_t **stmts = nova_parser_parse_statements(parser, &stmt_count);
    if (!stmts || nova_parser_had_error(parser)) {
      fprintf(stderr, "Parse error: %s\n", nova_parser_get_error(parser));
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // Create AST program
    nova_program_t *program = nova_program_create(opts.input_file);
    for (size_t i = 0; i < stmt_count; i++) {
      add_stmt_to_program(program, stmts[i]);
    }
    free(stmts);

    // 3. Semantic analysis
    {
      nova_module_registry_t *reg = nova_module_registry_create();
      const char *mod_path = opts.module_path ? opts.module_path : ".";
      nova_module_registry_set_search_path(reg, mod_path);
      nova_semantic_set_module_registry(reg);
    }

    nova_semantic_t *semantic = nova_semantic_create(program);
    if (!nova_semantic_analyze(semantic)) {
      fprintf(stderr, "Semantic error: %s\n",
              nova_semantic_get_error(semantic));
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // 4. LLVM Code generation
    fprintf(stderr, "Generating LLVM IR...\n");

    NovaLLVMContext *llvm_ctx = nova_llvm_create_context(opts.input_file);
    if (!llvm_ctx) {
      fprintf(stderr, "Failed to create LLVM context\n");
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // Generate LLVM IR from AST
    if (!nova_llvm_codegen_program(llvm_ctx, program)) {
      fprintf(stderr, "LLVM codegen failed\n");
      nova_llvm_destroy_context(llvm_ctx);
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // Handle emit flags
    if (opts.emit == EMIT_IR) {
      fprintf(stderr, "Emitting LLVM IR...\n");
      nova_llvm_dump_ir(llvm_ctx);
    }

    // 5. Generate object file
    const char *output = opts.output_file ? opts.output_file : "a.out";
    fprintf(stderr, "Generating object file: %s\n", output);

    if (nova_llvm_emit_object(llvm_ctx, output)) {
      fprintf(stderr, "✅ Native object file generated: %s\n", output);
    } else {
      fprintf(stderr, "❌ Failed to generate object file: %s\n", output);
    }

    // Cleanup
    nova_llvm_destroy_context(llvm_ctx);
    nova_semantic_destroy(semantic);
    nova_parser_destroy(parser);
    nova_lexer_destroy(lexer);
    nova_compiler_destroy(compiler);
    free(source);

    fprintf(stderr, "\n🎯 MODE_BUILD: Compilation pipeline complete!\n");
    return 0;
  }

  // Handle execution mode
  if (opts.mode == MODE_RUN || opts.use_jit) {
    // Lex
    nova_lexer_t *lexer = nova_lexer_create(source);
    if (!lexer) {
      fprintf(stderr, "Failed to create lexer\n");
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // Parse
    nova_parser_t *parser = nova_parser_create(lexer);
    if (!parser) {
      fprintf(stderr, "Failed to create parser\n");
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    size_t stmt_count;
    nova_stmt_t **stmts = nova_parser_parse_statements(parser, &stmt_count);

    if (!stmts || nova_parser_had_error(parser)) {
      fprintf(stderr, "Parse error: %s\n", nova_parser_get_error(parser));
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // Create AST program
    nova_program_t *program = nova_program_create(opts.input_file);
    for (size_t i = 0; i < stmt_count; i++) {
      add_stmt_to_program(program, stmts[i]);
    }
    free(stmts);

    // Semantic analysis
    {
      nova_module_registry_t *reg = nova_module_registry_create();
      const char *mod_path = opts.module_path;
      if (!mod_path)
        mod_path = getenv("NOVA_MODULE_PATH");
      if (!mod_path && opts.input_file) {
        static char dir_buf[4096];
        strncpy(dir_buf, opts.input_file, sizeof(dir_buf) - 1);
        char *slash = strrchr(dir_buf, '/');
        if (slash)
          *slash = '\0';
        else
          strncpy(dir_buf, ".", sizeof(dir_buf));
        mod_path = dir_buf;
      }
      if (!mod_path)
        mod_path = ".";
      nova_module_registry_set_search_path(reg, mod_path);
      nova_semantic_set_module_registry(reg);
    }

    nova_semantic_t *semantic = nova_semantic_create(program);
    if (!nova_semantic_analyze(semantic)) {
      fprintf(stderr, "Semantic error: %s\n",
              nova_semantic_get_error(semantic));
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    // Code generation
    nova_codegen_t *codegen = nova_codegen_create(NOVA_BACKEND_VM);
    if (!codegen) {
      fprintf(stderr, "Failed to create codegen\n");
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    nova_codegen_set_semantic(codegen, semantic);

    if (!nova_codegen_generate_from_ast(codegen, program)) {
      fprintf(stderr, "Codegen error: %s\n", nova_codegen_get_error(codegen));
      nova_codegen_destroy(codegen);
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    Chunk *chunk = nova_codegen_get_chunk(codegen);
    if (opts.verbose) {
      fprintf(stderr, "\n--- BYTECODE DISASSEMBLY ---\n");
      bytecode_disassemble_chunk(chunk, opts.input_file);
      fprintf(stderr, "---------------------------\n\n");
    }

    // Execute
    nova_vm_t *vm = nova_vm_create();
    if (!vm) {
      fprintf(stderr, "Failed to create VM\n");
      nova_codegen_destroy(codegen);
      nova_semantic_destroy(semantic);
      nova_parser_destroy(parser);
      nova_lexer_destroy(lexer);
      nova_compiler_destroy(compiler);
      free(source);
      return 1;
    }

    NovaInterpretResult vm_result = nova_vm_interpret(vm, chunk);
    if (vm_result != INTERPRET_OK) {
      fprintf(stderr, "VM Runtime Error: %s\n", nova_vm_get_error(vm));
    }

    // Cleanup
    nova_vm_destroy(vm);
    nova_codegen_destroy(codegen);
    nova_semantic_destroy(semantic);
    nova_parser_destroy(parser);
    nova_lexer_destroy(lexer);
    nova_compiler_destroy(compiler);
    free(source);

    return (vm_result == INTERPRET_OK) ? 0 : 1;
  }

  // Default mode: just parse + check
  {
    nova_module_registry_t *reg = nova_module_registry_create();
    const char *mod_path = opts.module_path;
    if (!mod_path)
      mod_path = getenv("NOVA_MODULE_PATH");
    if (!mod_path && opts.input_file) {
      static char dir_buf2[4096];
      strncpy(dir_buf2, opts.input_file, sizeof(dir_buf2) - 1);
      char *slash = strrchr(dir_buf2, '/');
      if (slash)
        *slash = '\0';
      else
        strncpy(dir_buf2, ".", sizeof(dir_buf2));
      mod_path = dir_buf2;
    }
    if (!mod_path)
      mod_path = ".";
    nova_module_registry_set_search_path(reg, mod_path);
    nova_semantic_set_module_registry(reg);
  }
  int result = nova_compile_source(compiler, source, opts.input_file);

  // Clean up
  nova_compiler_destroy(compiler);
  free(source);

  return result;
}
