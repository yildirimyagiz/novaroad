#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend/chunk.h"
#include "compiler/ast.h"
#include "compiler/borrow_checker.h"
#include "compiler/codegen.h"
#include "compiler/compiler.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic.h"
#include "compiler/sourcemgr.h"
#include "std/alloc.h"
#include "std/file_io.h"

// ============================================================================
// Compiler Structure
// ============================================================================

struct nova_compiler {
  nova_compiler_options_t options;
  nova_diag_collector_t *diagnostics;
  nova_sourcemgr_t *sourcemgr;
  Chunk *bytecode;
};

struct nova_compilation_unit {
  const char *name;
  void *output;
  size_t output_size;
};

// ============================================================================
// Compiler Options
// ============================================================================

nova_compiler_options_t nova_compiler_options_default(void) {
  nova_compiler_options_t opts = {
      .target = NOVA_TARGET_BYTECODE,
      .target_arch = "x86_64",
      .output_file = "output.o",
      .optimize = true,
      .opt_level = NOVA_OPT_BALANCED,
      .enable_inlining = true,
      .enable_vectorization = false,
      .emit_debug_info = true,
      .emit_source_maps = false,
      .verbose = false,
      .strict_mode = true,
      .unsafe_allowed = false,
      .experimental = false,
      .emit_ir = false,
      .emit_asm = false,
      .emit_llvm = false,
      .incremental = false,
      .cache_dir = NULL,
      .include_paths = NULL,
      .num_include_paths = 0,
  };
  return opts;
}

// ============================================================================
// Compiler Lifecycle
// ============================================================================

nova_compiler_t *nova_compiler_create(const nova_compiler_options_t *opts) {
  nova_compiler_t *compiler = nova_alloc(sizeof(nova_compiler_t));
  if (!compiler)
    return NULL;

  compiler->options = opts ? *opts : nova_compiler_options_default();

  compiler->diagnostics = nova_diag_collector_create();
  compiler->sourcemgr = nova_sourcemgr_create();
  compiler->bytecode = NULL;

  if (!compiler->diagnostics || !compiler->sourcemgr) {
    nova_compiler_destroy(compiler);
    return NULL;
  }

  return compiler;
}

nova_compiler_t *nova_compiler_create_default(void) {
  return nova_compiler_create(NULL);
}

void nova_compiler_destroy(nova_compiler_t *compiler) {
  if (!compiler)
    return;

  if (compiler->diagnostics) {
    nova_diag_collector_destroy(compiler->diagnostics);
  }
  if (compiler->sourcemgr) {
    nova_sourcemgr_destroy(compiler->sourcemgr);
  }
  if (compiler->bytecode) {
    chunk_free(compiler->bytecode);
    free(compiler->bytecode);
  }

  nova_free(compiler);
}

// ============================================================================
// Compilation
// ============================================================================

int nova_compile_file(nova_compiler_t *compiler, const char *filename) {
  if (!compiler || !filename)
    return -1;

  size_t length = 0;
  char *source = (char *)nova_file_read_all(filename, &length);
  if (!source) {
    return -1;
  }

  int result = nova_compile_source(compiler, source, filename);

  nova_free(source);
  return result;
}

int nova_compile_string(nova_compiler_t *compiler, const char *source) {
  return nova_compile_source(compiler, source, "<string>");
}

int nova_compile_files(nova_compiler_t *compiler, const char **files,
                       size_t count) {
  if (!compiler || !files)
    return -1;

  int total_result = 0;
  for (size_t i = 0; i < count; i++) {
    int result = nova_compile_file(compiler, files[i]);
    if (result != 0) {
      total_result = result;
    }
  }

  return total_result;
}

/*
 * Fixed implementation:
 * - Uses existing parser public API: nova_parser_parse_statements()
 * - Avoids non-existent types/functions (nova_program_t, nova_program_free,
 * nova_diag_error_count)
 * - Returns non-zero if parsing fails
 * - Frees parsed statements
 *
 * TODO (when APIs exist/are confirmed):
 * - semantic/typecheck entrypoint
 * - borrow checker entrypoint over program/stmt list
 * - codegen / emit-ir / emit-ast plumbing
 */
// ============================================================================
// Compilation Units (Stub Implementation)
// ============================================================================

nova_compilation_unit_t *nova_compiler_get_unit(nova_compiler_t *compiler,
                                                const char *name) {
  (void)compiler;
  (void)name;
  return NULL;
}

nova_compilation_unit_t **nova_compiler_get_units(nova_compiler_t *compiler,
                                                  size_t *count) {
  (void)compiler;
  if (count)
    *count = 0;
  return NULL;
}

const void *nova_compilation_unit_output(nova_compilation_unit_t *unit,
                                         size_t *size) {
  if (!unit || !size)
    return NULL;
  *size = unit->output_size;
  return unit->output;
}

// ============================================================================
// Pipeline Control (Stub Implementation)
// ============================================================================

void *nova_compiler_lex(nova_compiler_t *compiler, const char *source) {
  (void)compiler;
  return nova_lexer_create(source);
}

void *nova_compiler_parse(nova_compiler_t *compiler, const char *source) {
  (void)compiler;
  (void)source;
  return NULL;
}

int nova_compiler_typecheck(nova_compiler_t *compiler, void *ast) {
  (void)compiler;
  (void)ast;
  return 0;
}

void *nova_compiler_generate_ir(nova_compiler_t *compiler, void *ast) {
  (void)compiler;
  (void)ast;
  return NULL;
}

void *nova_compiler_optimize(nova_compiler_t *compiler, void *ir) {
  (void)compiler;
  (void)ir;
  return NULL;
}

int nova_compiler_codegen(nova_compiler_t *compiler, void *ir) {
  (void)compiler;
  (void)ir;
  return 0;
}

// ============================================================================
// Error Handling and Diagnostics (Stub Implementation)
// ============================================================================

const nova_diagnostic_t **
nova_compiler_get_diagnostics(nova_compiler_t *compiler, size_t *count) {
  (void)compiler;
  if (count)
    *count = 0;
  return NULL;
}

size_t nova_compiler_error_count(nova_compiler_t *compiler) {
  (void)compiler;
  return 0;
}

size_t nova_compiler_warning_count(nova_compiler_t *compiler) {
  (void)compiler;
  return 0;
}

void nova_compiler_clear_diagnostics(nova_compiler_t *compiler) {
  (void)compiler;
}

void nova_compiler_print_diagnostics(nova_compiler_t *compiler) {
  (void)compiler;
}

void nova_compiler_set_diagnostic_callback(
    nova_compiler_t *compiler,
    void (*callback)(const nova_diagnostic_t *diag, void *userdata),
    void *userdata) {
  (void)compiler;
  (void)callback;
  (void)userdata;
}

// ============================================================================
// Incremental Compilation (Stub Implementation)
// ============================================================================

void nova_compiler_mark_dirty(nova_compiler_t *compiler, const char *filename) {
  (void)compiler;
  (void)filename;
}

void nova_compiler_invalidate_cache(nova_compiler_t *compiler) {
  (void)compiler;
}

void nova_compiler_cache_stats(nova_compiler_t *compiler, size_t *hits,
                               size_t *misses) {
  (void)compiler;
  if (hits)
    *hits = 0;
  if (misses)
    *misses = 0;
}

// ============================================================================
// Compilation Statistics (Stub Implementation)
// ============================================================================

void nova_compiler_get_stats(nova_compiler_t *compiler,
                             nova_compiler_stats_t *stats) {
  (void)compiler;
  if (stats) {
    memset(stats, 0, sizeof(*stats));
  }
}

void nova_compiler_reset_stats(nova_compiler_t *compiler) { (void)compiler; }

// ============================================================================
// Output Generation (Stub Implementation)
// ============================================================================

int nova_compiler_write_output(nova_compiler_t *compiler,
                               const char *filename) {
  if (!compiler || !filename)
    return -1;
  if (!compiler->bytecode)
    return -1;

  chunk_save(compiler->bytecode, filename);
  return 0;
}

// Helper functions for nova_compile_source
static nova_program_t *nova_program_from_stmts(const char *filename,
                                               nova_stmt_t **stmts,
                                               size_t stmt_count,
                                               nova_diag_collector_t *diag) {
  nova_program_t *prog = (nova_program_t *)calloc(1, sizeof(*prog));
  if (!prog)
    return NULL;

  prog->filename = strdup(filename ? filename : "<source>");
  prog->declarations = NULL;
  prog->declaration_count = 0;

  for (size_t i = 0; i < stmt_count; i++) {
    nova_stmt_t *s = stmts[i];
    if (!s)
      continue;

    nova_decl_kind_t dk;
    switch (s->kind) {
    case STMT_FN:
      dk = DECL_FUNCTION;
      break;
    case STMT_STRUCT_DECL:
      dk = DECL_STRUCT;
      break;
    case STMT_ENUM_DECL:
      dk = DECL_ENUM;
      break;
    case STMT_VAR_DECL:
      dk = DECL_VAR;
      break;
    case STMT_MOD:
    case STMT_BLOCK: // Added to handle stubbed declarations like 'skill'
    case STMT_EXPR:  // Allow top-level expressions (like module exposures)
      continue;
    case STMT_IMPORT:
      /* Import statements are kept as top-level declarations so the
       * semantic analyzer can process them and register module symbols. */
      dk = DECL_VAR; /* Reuse existing kind — semantic checks stmt->kind */
      break;
    case STMT_WHILE:
    case STMT_FOR:
    case STMT_CHECK:
    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_RETURN:
    case STMT_HEAP_FREE:
    case STMT_YIELD:
      /* Allow top-level executable statements (script-style files) */
      dk = DECL_VAR; /* Reuse kind — semantic will handle execution */
      break;
    default:
      if (diag) {
        nova_diag_error(diag, s->span,
                        "Only declarations are allowed at top level");
      }
      continue;
    }

    nova_top_level_t *tl = (nova_top_level_t *)malloc(sizeof(*tl));
    if (!tl)
      continue;
    tl->kind = dk;
    tl->data = s;
    stmts[i] = NULL; // Ownership transferred to program

    prog->declarations = (nova_top_level_t **)realloc(
        prog->declarations,
        sizeof(nova_top_level_t *) * (prog->declaration_count + 1));
    prog->declarations[prog->declaration_count++] = tl;
  }

  return prog;
}

static void nova_program_free_local(nova_program_t *prog) {
  if (!prog)
    return;
  if (prog->declarations) {
    for (size_t i = 0; i < prog->declaration_count; i++) {
      nova_top_level_t *tl = prog->declarations[i];
      if (!tl)
        continue;
      prog->declarations[i] = NULL; /* prevent double-free */
      if (tl->data) {
        nova_stmt_free((nova_stmt_t *)tl->data);
        tl->data = NULL;
      }
      free(tl);
    }
    free(prog->declarations);
    prog->declarations = NULL;
  }
  if (prog->filename)
    free(prog->filename);
  free(prog);
}

int nova_compile_source(nova_compiler_t *compiler, const char *source,
                        const char *name) {
  if (!compiler || !source)
    return -1;
  if (!name)
    name = "<source>";

  nova_lexer_t *lexer = nova_lexer_create(source);
  if (!lexer)
    return -1;

  nova_parser_t *parser = nova_parser_create(lexer);
  if (!parser) {
    nova_lexer_destroy(lexer);
    return -1;
  }

  size_t stmt_count = 0;
  nova_stmt_t **stmts = nova_parser_parse_statements(parser, &stmt_count);

  int rc = 0;

  if (!stmts || nova_parser_had_error(parser)) {
    const char *err = nova_parser_get_error(parser);
    nova_token_t last_tok = nova_parser_get_last_token(parser);
    if (err && strcmp(err, "No error") != 0) {
      fprintf(stderr, "error: parse error in %s:%d:%d: %s\n", name,
              last_tok.line, last_tok.column, err);
    } else {
      fprintf(stderr, "error: parse error in %s:%d:%d\n", name, last_tok.line,
              last_tok.column);
    }
    rc = 1;
    goto cleanup;
  }

  nova_program_t *program =
      nova_program_from_stmts(name, stmts, stmt_count, compiler->diagnostics);
  if (!program) {
    fprintf(stderr, "error: out of memory building program AST\n");
    rc = 1;
    goto cleanup;
  }

  nova_semantic_t *sem = nova_semantic_create(program);
  if (!sem) {
    fprintf(stderr, "error: failed to create semantic analyzer\n");
    rc = 1;
  } else {
    if (!nova_semantic_analyze(sem)) {
      const char *msg = nova_semantic_get_error(sem);
      fprintf(stderr, "error: %s\n", msg ? msg : "semantic analysis failed");
      rc = 1;
    }
    nova_semantic_destroy(sem);
  }

  // Always run borrow checker
  if (rc == 0) {
    nova_borrow_checker_t *bc =
        nova_borrow_checker_create(compiler->diagnostics);
    if (bc) {
      if (!nova_borrow_check_program(bc, program)) {
        fprintf(stderr, "error: borrow check failed\n");
        rc = 1;
      }
      nova_borrow_checker_destroy(bc);
    }
  }

#include "compiler/semantic.h" // Ensure semantic is included for codegen
  // Codegen
  if (rc == 0) {
    nova_backend_type_t b_type = (nova_backend_type_t)compiler->options.target;
    nova_codegen_t *codegen = nova_codegen_create(b_type);
    if (codegen) {
      // Re-create semantic context for codegen if needed
      // (In a real impl, we'd pass it along)
      nova_semantic_t *sem_cg = nova_semantic_create(program);
      nova_codegen_set_semantic(codegen, sem_cg);

      if (nova_codegen_generate_from_ast(codegen, program)) {
        Chunk *cg_chunk = nova_codegen_get_chunk(codegen);
        if (cg_chunk) {
          if (compiler->bytecode) {
            chunk_free(compiler->bytecode);
            free(compiler->bytecode);
          }
          compiler->bytecode = (Chunk *)malloc(sizeof(Chunk));
          chunk_init(compiler->bytecode);
          // Copy chunk data (simple copy for now)
          for (int i = 0; i < cg_chunk->count; i++) {
            chunk_write(compiler->bytecode, cg_chunk->code[i],
                        cg_chunk->lines.lines[i]);
          }
          // Copy constants
          for (int i = 0; i < cg_chunk->constants.count; i++) {
            chunk_add_constant(compiler->bytecode,
                               cg_chunk->constants.values[i]);
          }
        }
      } else {
        fprintf(stderr, "error: codegen failed: %s\n",
                nova_codegen_get_error(codegen));
        rc = 1;
      }
      if (sem_cg)
        nova_semantic_destroy(sem_cg);
      nova_codegen_destroy(codegen);
    }
  }

  nova_diag_print(compiler->diagnostics, compiler->sourcemgr);
  nova_program_free_local(program);

cleanup:
  // Don't free stmts here if they were transferred to program
  // (program will free them in nova_program_free_local)
  if (stmts && !program) {
    for (size_t i = 0; i < stmt_count; i++) {
      if (stmts[i])
        nova_stmt_free(stmts[i]);
    }
    free(stmts);
  } else if (stmts) {
    // Just free the array, not the contents (program owns them)
    free(stmts);
  }

  nova_parser_destroy(parser);
  nova_lexer_destroy(lexer);
  return rc;
}

const void *nova_compiler_get_output(nova_compiler_t *compiler, size_t *size) {
  if (!compiler || !compiler->bytecode) {
    if (size)
      *size = 0;
    return NULL;
  }
  if (size)
    *size = compiler->bytecode->count;
  return compiler->bytecode->code;
}
