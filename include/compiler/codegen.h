/**
 * @file codegen.h
 * @brief Code generation backends
 */

#ifndef NOVA_CODEGEN_H
#define NOVA_CODEGEN_H

#include "ir.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"

typedef struct nova_codegen nova_codegen_t;
typedef struct nova_ir_module nova_ir_module_t;

nova_codegen_t *nova_codegen_create(nova_backend_type_t backend);
int nova_codegen_generate(nova_codegen_t *codegen, nova_ir_module_t *ir_module);
int nova_codegen_write(nova_codegen_t *codegen, const char *filename);
const void *nova_codegen_get_output(nova_codegen_t *codegen, size_t *size);
void nova_codegen_destroy(nova_codegen_t *codegen);
const char *nova_codegen_get_error(nova_codegen_t *codegen);

/* AST-based codegen (legacy/bootstrap) */
#include "ast.h"
#include "../backend/chunk.h"
// semantic.h removed as it is not used directly here
bool nova_codegen_generate_from_ast(nova_codegen_t *codegen,
                                    nova_program_t *ast);
void nova_codegen_set_semantic(nova_codegen_t *codegen,
                               nova_semantic_t *semantic);
Chunk *nova_codegen_get_chunk(nova_codegen_t *codegen);

/* Backend-specific options */
void nova_codegen_set_option(nova_codegen_t *codegen, const char *key,
                             const char *value);
const char *nova_codegen_get_option(nova_codegen_t *codegen, const char *key);

#ifdef __cplusplus
}
#endif
#endif /* NOVA_CODEGEN_H */
