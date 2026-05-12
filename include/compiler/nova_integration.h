/**
 * @file nova_integration.h
 * @brief Nova .zn syntax integration with C backend
 */

#ifndef NOVA_INTEGRATION_H
#define NOVA_INTEGRATION_H

#include "./codegen.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct nova_ir_module nova_ir_module_t;
typedef struct nova_ast_node nova_ast_node_t;

typedef struct nova_integration_context {
  nova_ast_node_t *zn_ast; // Original .zn AST
  nova_ast_node_t *c_ast;  // Lowered C AST
  nova_ir_module_t *ir;    // IR module
  void *codegen;           // Code generator
  char error_msg[256];     // Error message buffer
} nova_integration_context_t;

/**
 * Create integration context
 */
nova_integration_context_t *nova_integration_create(void);

/**
 * Parse Nova .zn file
 */
int nova_parse_zn_file(nova_integration_context_t *ctx, const char *filename);

/**
 * Lower Nova AST to C AST
 */
int nova_lower_to_c_ast(nova_integration_context_t *ctx);

/**
 * Generate IR from AST
 */
int nova_generate_ir(nova_integration_context_t *ctx);

/**
 * Compile to target backend
 */
int nova_compile_to_backend(nova_integration_context_t *ctx,
                            nova_backend_type_t backend);

/**
 * Full compilation pipeline: .zn → AST → IR → Backend
 *
 * @param input_file Nova source file (.zn)
 * @param output_file Output file (depends on backend)
 * @param backend Target backend (LLVM, JIT, WASM, etc.)
 * @return 0 on success, -1 on error
 */
int nova_compile_zn_file(const char *input_file, const char *output_file,
                         nova_backend_type_t backend);

/**
 * Add a search path for modules
 */
void nova_integration_add_search_path(const char *path);

/**
 * Get error message
 */
const char *nova_integration_get_error(nova_integration_context_t *ctx);

/**
 * Destroy integration context
 */
void nova_integration_destroy(nova_integration_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_INTEGRATION_H */
