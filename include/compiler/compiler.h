/**
 * @file compiler.h
 * @brief Nova compiler interface and compilation pipeline
 *
 * The Nova compiler provides:
 * - Multi-stage compilation pipeline (lex → parse → typecheck → IR → optimize →
 * codegen)
 * - Multiple backends (bytecode VM, JIT, LLVM, WASM)
 * - Incremental compilation
 * - Error reporting and diagnostics
 * - Optimization levels
 * - Debug information generation
 */

#ifndef NOVA_COMPILER_H
#define NOVA_COMPILER_H

#include "../platform/platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Compiler Types
 * ======================================================================== */

typedef struct nova_compiler nova_compiler_t;
typedef struct nova_compilation_unit nova_compilation_unit_t;
typedef struct nova_diagnostic nova_diagnostic_t;

/**
 * Compilation Backend Target
 */
#ifndef NOVA_BACKEND_TYPE_DEFINED
#define NOVA_BACKEND_TYPE_DEFINED
typedef enum {
  NOVA_BACKEND_VM,            /**< Bytecode for VM */
  NOVA_BACKEND_JIT,           /**< JIT compilation */
  NOVA_BACKEND_LLVM,          /**< LLVM IR */
  NOVA_BACKEND_WASM,          /**< WebAssembly */
  NOVA_BACKEND_NATIVE,        /**< Native executable */
  NOVA_BACKEND_MLIR_CODEGEN,  /**< MLIR IR Code Generation */
  NOVA_BACKEND_GPU_ARMY_4LUA, /**< 4-Layer Unified Army (Tiered Compute) */
} nova_backend_type_t;

/* Legacy architecture/backend aliases */
#define NOVA_TARGET_BYTECODE NOVA_BACKEND_VM
#define NOVA_TARGET_JIT NOVA_BACKEND_JIT
#define NOVA_TARGET_LLVM NOVA_BACKEND_LLVM
#define NOVA_TARGET_WASM NOVA_BACKEND_WASM
#define NOVA_TARGET_NATIVE NOVA_BACKEND_NATIVE

typedef nova_backend_type_t NovaBackendType;
#endif

/**
 * Target Architecture
 */
typedef nova_arch_type_t NovaTargetArch;

/* Legacy architecture aliases */
#define NOVA_TARGET_X86_64 NOVA_ARCH_TYPE_X86_64
#define NOVA_TARGET_ARM64 NOVA_ARCH_TYPE_AARCH64
#define NOVA_TARGET_RISCV NOVA_ARCH_TYPE_RISCV64
// Note: NOVA_TARGET_WASM and NOVA_TARGET_NATIVE collide with backends.
// Code should use NOVA_ARCH_TYPE_WASM32 or NOVA_ARCH_TYPE_WASM64 or handle contextually.

/**
 * Optimization Level
 */
#ifndef NOVA_OPT_LEVEL_DEFINED
#define NOVA_OPT_LEVEL_DEFINED
typedef enum {
  NOVA_OPT_NONE,       /**< No optimization */
  NOVA_OPT_MINIMAL,    /**< Minimal optimization (O1) */
  NOVA_OPT_BALANCED,   /**< Balanced (O2) */
  NOVA_OPT_AGGRESSIVE, /**< Aggressive (O3) */
  NOVA_OPT_SIZE,       /**< Optimize for size (Os) */
} nova_opt_level_t;

typedef nova_opt_level_t NovaOptLevel;

/* LLVM backend aliases */
#define NOVA_OPT_O0 NOVA_OPT_NONE
#define NOVA_OPT_O1 NOVA_OPT_MINIMAL
#define NOVA_OPT_O2 NOVA_OPT_BALANCED
#define NOVA_OPT_O3 NOVA_OPT_AGGRESSIVE
#define NOVA_OPT_Oz NOVA_OPT_SIZE
#endif

/**
 * Compiler options
 */
typedef struct {
  /* Target settings */
  nova_backend_type_t target; /**< Compilation target */
  const char *target_arch; /**< Target architecture (x86_64, aarch64, etc.) */
  const char *output_file; /**< Output file path */

  /* Optimization settings */
  bool optimize;              /**< Enable optimization */
  nova_opt_level_t opt_level; /**< Optimization level */
  bool enable_inlining;       /**< Enable function inlining */
  bool enable_vectorization;  /**< Enable SIMD vectorization */

  /* Debug settings */
  bool emit_debug_info;  /**< Generate debug information */
  bool emit_source_maps; /**< Generate source maps */
  bool verbose;          /**< Verbose output */

  /* Language features */
  bool strict_mode;    /**< Enable strict type checking */
  bool unsafe_allowed; /**< Allow unsafe code blocks */
  bool experimental;   /**< Enable experimental features */

  /* Code generation */
  bool emit_ir;   /**< Emit IR to file */
  bool emit_asm;  /**< Emit assembly to file */
  bool emit_llvm; /**< Emit LLVM IR to file */

  /* Incremental compilation */
  bool incremental;      /**< Enable incremental compilation */
  const char *cache_dir; /**< Cache directory for incremental builds */

  /* Include paths */
  const char **include_paths; /**< Array of include directories */
  size_t num_include_paths;   /**< Number of include paths */
} nova_compiler_options_t;

/**
 * @brief Get default compiler options
 * @return Default options structure
 */
nova_compiler_options_t nova_compiler_options_default(void);

/* ========================================================================
 * Compiler Lifecycle
 * ======================================================================== */

/**
 * @brief Create compiler instance
 * @param opts Compiler options
 * @return Compiler handle, or NULL on failure
 */
nova_compiler_t *nova_compiler_create(const nova_compiler_options_t *opts);

/**
 * @brief Create compiler with default options
 * @return Compiler handle, or NULL on failure
 */
nova_compiler_t *nova_compiler_create_default(void);

/**
 * @brief Destroy compiler
 * @param compiler Compiler handle
 */
void nova_compiler_destroy(nova_compiler_t *compiler);

/* ========================================================================
 * Compilation
 * ======================================================================== */

/**
 * @brief Compile source file
 * @param compiler Compiler handle
 * @param filename Source file path
 * @return 0 on success, negative error code on failure
 */
int nova_compile_file(nova_compiler_t *compiler, const char *filename);

/**
 * @brief Compile source string
 * @param compiler Compiler handle
 * @param source Source code string
 * @return 0 on success, negative error code on failure
 */
int nova_compile_string(nova_compiler_t *compiler, const char *source);

/**
 * @brief Compile multiple files
 * @param compiler Compiler handle
 * @param files Array of file paths
 * @param count Number of files
 * @return 0 on success, negative error code on failure
 */
int nova_compile_files(nova_compiler_t *compiler, const char **files,
                       size_t count);

/**
 * @brief Compile source with custom name (for REPL/eval)
 * @param compiler Compiler handle
 * @param source Source code string
 * @param name Source name (for error messages)
 * @return 0 on success, negative error code on failure
 */
int nova_compile_source(nova_compiler_t *compiler, const char *source,
                        const char *name);

/* ========================================================================
 * Compilation Units
 * ======================================================================== */

/**
 * @brief Get compilation unit
 * @param compiler Compiler handle
 * @param name Module/file name
 * @return Compilation unit, or NULL if not found
 */
nova_compilation_unit_t *nova_compiler_get_unit(nova_compiler_t *compiler,
                                                const char *name);

/**
 * @brief Get all compilation units
 * @param compiler Compiler handle
 * @param count Output: number of units
 * @return Array of compilation units
 */
nova_compilation_unit_t **nova_compiler_get_units(nova_compiler_t *compiler,
                                                  size_t *count);

/**
 * @brief Get compilation unit output
 * @param unit Compilation unit
 * @param size Output: size in bytes
 * @return Output buffer (bytecode, machine code, etc.)
 */
const void *nova_compilation_unit_output(nova_compilation_unit_t *unit,
                                         size_t *size);

/* ========================================================================
 * Pipeline Control
 * ======================================================================== */

/**
 * @brief Run only lexical analysis
 * @param compiler Compiler handle
 * @param source Source code
 * @return Token stream, or NULL on failure
 */
void *nova_compiler_lex(nova_compiler_t *compiler, const char *source);

/**
 * @brief Run lexer and parser
 * @param compiler Compiler handle
 * @param source Source code
 * @return AST, or NULL on failure
 */
void *nova_compiler_parse(nova_compiler_t *compiler, const char *source);

/**
 * @brief Run type checking on AST
 * @param compiler Compiler handle
 * @param ast AST to check
 * @return 0 on success, negative on error
 */
int nova_compiler_typecheck(nova_compiler_t *compiler, void *ast);

/**
 * @brief Generate IR from AST
 * @param compiler Compiler handle
 * @param ast Typed AST
 * @return IR module, or NULL on failure
 */
void *nova_compiler_generate_ir(nova_compiler_t *compiler, void *ast);

/**
 * @brief Optimize IR
 * @param compiler Compiler handle
 * @param ir IR module
 * @return Optimized IR, or NULL on failure
 */
void *nova_compiler_optimize(nova_compiler_t *compiler, void *ir);

/**
 * @brief Generate code from IR
 * @param compiler Compiler handle
 * @param ir IR module
 * @return 0 on success, negative on failure
 */
int nova_compiler_codegen(nova_compiler_t *compiler, void *ir);

/* ========================================================================
 * Error Handling and Diagnostics
 * ======================================================================== */

/**
 * Diagnostic severity
 */
typedef enum {
  NOVA_DIAG_NOTE,    /**< Informational note */
  NOVA_DIAG_WARNING, /**< Warning (non-fatal) */
  NOVA_DIAG_ERROR,   /**< Error (fatal) */
} nova_diag_severity_t;

/**
 * Diagnostic structure
 */
struct nova_diagnostic {
  nova_diag_severity_t severity; /**< Diagnostic severity */
  const char *message;           /**< Error message */
  const char *filename;          /**< Source file */
  int line;                      /**< Line number */
  int column;                    /**< Column number */
  const char *source_line;       /**< Source line text */
};

/**
 * @brief Get compilation diagnostics
 * @param compiler Compiler handle
 * @param count Output: number of diagnostics
 * @return Array of diagnostics
 */
const nova_diagnostic_t **
nova_compiler_get_diagnostics(nova_compiler_t *compiler, size_t *count);

/**
 * @brief Get error count
 * @param compiler Compiler handle
 * @return Number of errors
 */
size_t nova_compiler_error_count(nova_compiler_t *compiler);

/**
 * @brief Get warning count
 * @param compiler Compiler handle
 * @return Number of warnings
 */
size_t nova_compiler_warning_count(nova_compiler_t *compiler);

/**
 * @brief Clear diagnostics
 * @param compiler Compiler handle
 */
void nova_compiler_clear_diagnostics(nova_compiler_t *compiler);

/**
 * @brief Print diagnostics to stderr
 * @param compiler Compiler handle
 */
void nova_compiler_print_diagnostics(nova_compiler_t *compiler);

/**
 * @brief Set diagnostic callback
 * @param compiler Compiler handle
 * @param callback Callback function
 * @param userdata User data passed to callback
 */
void nova_compiler_set_diagnostic_callback(
    nova_compiler_t *compiler,
    void (*callback)(const nova_diagnostic_t *diag, void *userdata),
    void *userdata);

/* ========================================================================
 * Incremental Compilation
 * ======================================================================== */

/**
 * @brief Mark file as modified (for incremental compilation)
 * @param compiler Compiler handle
 * @param filename File path
 */
void nova_compiler_mark_dirty(nova_compiler_t *compiler, const char *filename);

/**
 * @brief Invalidate compilation cache
 * @param compiler Compiler handle
 */
void nova_compiler_invalidate_cache(nova_compiler_t *compiler);

/**
 * @brief Get compilation cache statistics
 * @param compiler Compiler handle
 * @param hits Output: cache hits
 * @param misses Output: cache misses
 */
void nova_compiler_cache_stats(nova_compiler_t *compiler, size_t *hits,
                               size_t *misses);

/* ========================================================================
 * Compilation Statistics
 * ======================================================================== */

typedef struct {
  uint64_t compile_time_ms;   /**< Total compilation time */
  uint64_t lex_time_ms;       /**< Lexing time */
  uint64_t parse_time_ms;     /**< Parsing time */
  uint64_t typecheck_time_ms; /**< Type checking time */
  uint64_t ir_gen_time_ms;    /**< IR generation time */
  uint64_t optimize_time_ms;  /**< Optimization time */
  uint64_t codegen_time_ms;   /**< Code generation time */

  size_t lines_compiled;  /**< Lines of code compiled */
  size_t ast_nodes;       /**< AST nodes generated */
  size_t ir_instructions; /**< IR instructions generated */
  size_t output_size;     /**< Output size in bytes */
} nova_compiler_stats_t;

/**
 * @brief Get compilation statistics
 * @param compiler Compiler handle
 * @param stats Output: statistics structure
 */
void nova_compiler_get_stats(nova_compiler_t *compiler,
                             nova_compiler_stats_t *stats);

/**
 * @brief Reset statistics
 * @param compiler Compiler handle
 */
void nova_compiler_reset_stats(nova_compiler_t *compiler);

/* ========================================================================
 * Output Generation
 * ======================================================================== */

/**
 * @brief Write output to file
 * @param compiler Compiler handle
 * @param filename Output file path
 * @return 0 on success, -1 on failure
 */
int nova_compiler_write_output(nova_compiler_t *compiler, const char *filename);

/**
 * @brief Get output buffer
 * @param compiler Compiler handle
 * @param size Output: buffer size
 * @return Output buffer (caller should not free)
 */
const void *nova_compiler_get_output(nova_compiler_t *compiler, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_COMPILER_H */
