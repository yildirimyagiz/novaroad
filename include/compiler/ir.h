/**
 * @file ir.h
 * @brief Intermediate Representation (SSA-form IR)
 */

#ifndef NOVA_COMPILER_IR_H
#define NOVA_COMPILER_IR_H


#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IR_NOP, IR_LOAD, IR_STORE, IR_LOAD_CONST,
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_AND, IR_OR, IR_XOR, IR_NOT, IR_NEG,
    IR_EQ, IR_NE, IR_LT, IR_LE, IR_GT, IR_GE,
    IR_CALL, IR_RET, IR_ALLOCA,
    IR_BR, IR_BR_COND, IR_SWITCH,
    IR_PHI, IR_CAST, IR_BITCAST,
} nova_ir_opcode_t;

typedef struct nova_ir_instr {
    nova_ir_opcode_t opcode;
    uint32_t dest;
    uint32_t src1;
    uint32_t src2;
    void *metadata;
    struct nova_ir_instr *next;
} nova_ir_instr_t;

typedef struct nova_ir_basic_block nova_ir_basic_block_t;
typedef struct nova_ir_function nova_ir_function_t;
typedef struct nova_ir_module nova_ir_module_t;

nova_ir_module_t *nova_ir_module_create(const char *name);
nova_ir_function_t *nova_ir_function_create(nova_ir_module_t *mod, const char *name);
nova_ir_basic_block_t *nova_ir_block_create(nova_ir_function_t *func);
void nova_ir_add_instr(nova_ir_basic_block_t *block, nova_ir_instr_t *instr);
void nova_ir_print(nova_ir_module_t *module);
void nova_ir_destroy(nova_ir_module_t *module);

#ifdef __cplusplus
}
#endif
#endif /* NOVA_IR_H */
