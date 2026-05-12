#include <stdbool.h>

/**
 * @file nova_integration.c
 * @brief Integration layer between Nova .zn syntax and C backend (STUB)
 */

#include "compiler/ast.h"
#include "compiler/compiler.h"
#include "compiler/ir.h"
#include "compiler/nova_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// STUB IMPLEMENTATIONS
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Stub: Create integration context
 * TODO: Implement when .zn integration is needed
 */
nova_integration_context_t *nova_integration_create(void) {
  nova_integration_context_t *ctx = malloc(sizeof(nova_integration_context_t));
  if (!ctx)
    return NULL;

  memset(ctx, 0, sizeof(nova_integration_context_t));
  return ctx;
}

/**
 * Stub: Convert .zn AST to C AST
 * TODO: Implement AST conversion
 */
int nova_convert_zn_to_c_ast(nova_integration_context_t *ctx, void *zn_ast) {
  (void)ctx;
  (void)zn_ast;
  return 0; // Success stub
}

/**
 * Stub: Generate IR from AST
 * TODO: Implement IR generation
 */
int nova_generate_ir(nova_integration_context_t *ctx) {
  if (!ctx)
    return -1;

  ctx->ir = nova_ir_module_create("module");
  return 0;
}

/**
 * Stub: Compile to backend
 * TODO: Implement backend compilation
 */
int nova_compile_to_backend(nova_integration_context_t *ctx,
                            nova_backend_type_t backend) {
  (void)ctx;
  (void)backend;
  return 0; // Success stub
}

/**
 * Stub: Full compilation pipeline
 */
int nova_compile_zn_file(const char *input_file, const char *output_file,
                         nova_backend_type_t backend) {
  nova_compiler_options_t opts = nova_compiler_options_default();
  opts.output_file = output_file;
  opts.target = backend;

  nova_compiler_t *compiler = nova_compiler_create(&opts);
  if (!compiler)
    return -1;

  int result = nova_compile_file(compiler, input_file);

  if (result == 0) {
    result = nova_compiler_write_output(compiler, output_file);
  }

  nova_compiler_destroy(compiler);
  return result;
}

/**
 * Stub: Destroy integration context
 */
void nova_integration_destroy(nova_integration_context_t *ctx) {
  if (!ctx)
    return;

  // if (ctx->ir) nova_ir_destroy(ctx->ir);
  free(ctx);
}

/**
 * Stub: Get error message
 */
const char *nova_integration_get_error(nova_integration_context_t *ctx) {
  return ctx ? ctx->error_msg : "No context";
}

#include "compiler/module_registry.h"
#include "compiler/semantic.h"

void nova_integration_add_search_path(const char *path) {
  nova_module_registry_t *reg = nova_semantic_get_module_registry();
  if (reg) {
    nova_module_registry_set_search_path(reg, path);
  }
}
