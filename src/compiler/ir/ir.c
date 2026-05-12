/**
 * @file ir.c
 * @brief Intermediate Representation implementation
 */

#include "compiler/ir.h"
#include "std/alloc.h"
#include <stdio.h>
#include <stdlib.h>

struct nova_ir_module {
  nova_ir_instr_t *instructions;
  nova_ir_instr_t *tail;
  size_t instr_count;
};

nova_ir_module_t *nova_ir_module_create(const char *name) {
  (void)name;
  nova_ir_module_t *module = nova_alloc(sizeof(nova_ir_module_t));
  if (!module)
    return NULL;

  module->instructions = NULL;
  module->tail = NULL;
  module->instr_count = 0;

  return module;
}

struct nova_ir_basic_block {
  nova_ir_instr_t *instructions;
  nova_ir_instr_t *tail;
  size_t instr_count;
};

void nova_ir_add_instr(nova_ir_basic_block_t *block, nova_ir_instr_t *instr) {
  if (!block || !instr)
    return;

  instr->next = NULL;

  if (!block->instructions) {
    block->instructions = instr;
    block->tail = instr;
  } else {
    block->tail->next = instr;
    block->tail = instr;
  }

  block->instr_count++;
}

void nova_ir_print(nova_ir_module_t *module) {
  if (!module)
    return;

  printf("IR Module (%zu instructions):\n", module->instr_count);

  nova_ir_instr_t *instr = module->instructions;
  size_t idx = 0;

  while (instr) {
    printf("  %zu: ", idx++);

    switch (instr->opcode) {
    case IR_NOP:
      printf("nop\n");
      break;
    case IR_LOAD:
      printf("%%r%u = load %%r%u\n", instr->dest, instr->src1);
      break;
    case IR_STORE:
      printf("store %%r%u, %%r%u\n", instr->src1, instr->dest);
      break;
    case IR_ADD:
      printf("%%r%u = add %%r%u, %%r%u\n", instr->dest, instr->src1,
             instr->src2);
      break;
    case IR_SUB:
      printf("%%r%u = sub %%r%u, %%r%u\n", instr->dest, instr->src1,
             instr->src2);
      break;
    case IR_MUL:
      printf("%%r%u = mul %%r%u, %%r%u\n", instr->dest, instr->src1,
             instr->src2);
      break;
    case IR_DIV:
      printf("%%r%u = div %%r%u, %%r%u\n", instr->dest, instr->src1,
             instr->src2);
      break;
    case IR_CALL:
      printf("%%r%u = call @func%u(%%r%u)\n", instr->dest, instr->src1,
             instr->src2);
      break;
    case IR_RET:
      printf("ret %%r%u\n", instr->src1);
      break;
    case IR_BR:
      printf("br label%%%u\n", instr->src1);
      break;
    case IR_BR_COND:
      printf("br %%r%u, label%%%u, label%%%u\n", instr->dest, instr->src1,
             instr->src2);
      break;
    default:
      printf("unknown\n");
    }

    instr = instr->next;
  }
}

void nova_ir_destroy(nova_ir_module_t *module) {
  if (!module)
    return;

  nova_ir_instr_t *instr = module->instructions;
  while (instr) {
    nova_ir_instr_t *next = instr->next;
    nova_free(instr);
    instr = next;
  }

  nova_free(module);
}
