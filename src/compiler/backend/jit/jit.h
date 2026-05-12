/**
 * @file jit.h
 * @brief JIT compiler internal header
 */

#ifndef NOVA_JIT_INTERNAL_H
#define NOVA_JIT_INTERNAL_H

#include "../../../../include/compiler/nova_ir.h"


#include <stdint.h>
#include <stddef.h>

typedef struct nova_jit_compiler nova_jit_compiler_t;
typedef struct jit_buffer jit_buffer_t;

/* JIT compiler functions */
nova_jit_compiler_t *nova_jit_create(void);
int nova_jit_compile(nova_jit_compiler_t *jit, IRModule *ir);

void *nova_jit_get_function(nova_jit_compiler_t *jit);
void nova_jit_destroy(nova_jit_compiler_t *jit);

/* Emit helpers */
void nova_jit_emit_byte(nova_jit_compiler_t *jit, uint8_t byte);
void nova_jit_emit_bytes(nova_jit_compiler_t *jit, const uint8_t *bytes, size_t len);
jit_buffer_t *nova_jit_get_buffer(nova_jit_compiler_t *jit);

#endif /* NOVA_JIT_INTERNAL_H */
