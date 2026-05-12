/**
 * @file vm.h
 * @brief Nova Virtual Machine
 */

#ifndef NOVA_VM_H
#define NOVA_VM_H

#include "./chunk.h"

#define STACK_MAX 4096 // Increased from 256 to support deeper recursion

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} NovaInterpretResult;

typedef struct nova_vm nova_vm_t;

/**
 * Create VM
 */
nova_vm_t *nova_vm_create(void);

/**
 * Run bytecode
 */
NovaInterpretResult nova_vm_interpret(nova_vm_t *vm, Chunk *chunk);

/**
 * Get error message
 */
const char *nova_vm_get_error(nova_vm_t *vm);

/**
 * Destroy VM
 */
void nova_vm_destroy(nova_vm_t *vm);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_VM_H */
