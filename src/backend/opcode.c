/**
 * @file opcode.c
 * @brief Implementation of opcode utility functions
 */

#include "backend/opcode.h"
#include <stddef.h>

/// Opcode name table for debugging
static const char *opcode_names[] = {
    // Constants
    [OP_CONSTANT] = "OP_CONSTANT",
    [OP_NULL] = "OP_NULL",
    [OP_TRUE] = "OP_TRUE",
    [OP_FALSE] = "OP_FALSE",

    // Arithmetic
    [OP_ADD] = "OP_ADD",
    [OP_SUBTRACT] = "OP_SUBTRACT",
    [OP_MULTIPLY] = "OP_MULTIPLY",
    [OP_DIVIDE] = "OP_DIVIDE",
    [OP_NEGATE] = "OP_NEGATE",
    [OP_MODULO] = "OP_MODULO",

    // Comparison
    [OP_EQUAL] = "OP_EQUAL",
    [OP_NOT_EQUAL] = "OP_NOT_EQUAL",
    [OP_LESS] = "OP_LESS",
    [OP_LESS_EQUAL] = "OP_LESS_EQUAL",
    [OP_GREATER] = "OP_GREATER",
    [OP_GREATER_EQUAL] = "OP_GREATER_EQUAL",

    // Logical
    [OP_NOT] = "OP_NOT",
    [OP_AND] = "OP_AND",
    [OP_OR] = "OP_OR",

    // Control flow
    [OP_JUMP] = "OP_JUMP",
    [OP_JUMP_IF_FALSE] = "OP_JUMP_IF_FALSE",
    [OP_JUMP_IF_TRUE] = "OP_JUMP_IF_TRUE",
    [OP_LOOP] = "OP_LOOP",
    [OP_CALL] = "OP_CALL",
    [OP_VPU_CALL] = "OP_VPU_CALL",
    [OP_RETURN] = "OP_RETURN",

    // Variables
    [OP_DEFINE_GLOBAL] = "OP_DEFINE_GLOBAL",
    [OP_GET_GLOBAL] = "OP_GET_GLOBAL",
    [OP_SET_GLOBAL] = "OP_SET_GLOBAL",
    [OP_GET_LOCAL] = "OP_GET_LOCAL",
    [OP_SET_LOCAL] = "OP_SET_LOCAL",
    [OP_GET_UPVALUE] = "OP_GET_UPVALUE",
    [OP_SET_UPVALUE] = "OP_SET_UPVALUE",

    // Functions
    [OP_CLOSURE] = "OP_CLOSURE",
    [OP_CLOSE_UPVALUE] = "OP_CLOSE_UPVALUE",

    // Objects
    [OP_CLASS] = "OP_CLASS",
    [OP_GET_PROPERTY] = "OP_GET_PROPERTY",
    [OP_SET_PROPERTY] = "OP_SET_PROPERTY",
    [OP_GET_METHOD] = "OP_GET_METHOD",
    [OP_INVOKE] = "OP_INVOKE",
    [OP_INHERIT] = "OP_INHERIT",

    // Memory
    [OP_POP] = "OP_POP",
    [OP_DUP] = "OP_DUP",
    [OP_PRINT] = "OP_PRINT",

    // Pointers
    [OP_ADDR_OF] = "OP_ADDR_OF",
    [OP_DEREF] = "OP_DEREF",
    [OP_STORE_PTR] = "OP_STORE_PTR",

    // Array Operations
    [OP_ARRAY_LIT] = "OP_ARRAY_LIT",
    [OP_INDEX_GET] = "OP_INDEX_GET",
    [OP_INDEX_SET] = "OP_INDEX_SET",
    [OP_ARRAY_LEN] = "OP_ARRAY_LEN",
    [OP_ENUM] = "OP_ENUM",
    [OP_GET_TAG] = "OP_GET_TAG",
    [OP_GET_FIELD] = "OP_GET_FIELD",

    // Special
    [OP_EOF] = "OP_EOF",
};

/// Operand count table
static const int operand_counts[] = {
    // Constants
    [OP_CONSTANT] = 1, // constant index
    [OP_NULL] = 0,
    [OP_TRUE] = 0,
    [OP_FALSE] = 0,

    // Arithmetic
    [OP_ADD] = 0,
    [OP_SUBTRACT] = 0,
    [OP_MULTIPLY] = 0,
    [OP_DIVIDE] = 0,
    [OP_NEGATE] = 0,
    [OP_MODULO] = 0,

    // Comparison
    [OP_EQUAL] = 0,
    [OP_NOT_EQUAL] = 0,
    [OP_LESS] = 0,
    [OP_LESS_EQUAL] = 0,
    [OP_GREATER] = 0,
    [OP_GREATER_EQUAL] = 0,

    // Logical
    [OP_NOT] = 0,
    [OP_AND] = 0,
    [OP_OR] = 0,

    // Control flow
    [OP_JUMP] = 2, // 16-bit offset
    [OP_JUMP_IF_FALSE] = 2,
    [OP_JUMP_IF_TRUE] = 2,
    [OP_LOOP] = 2,
    [OP_CALL] = 1,     // arg count
    [OP_VPU_CALL] = 4, // backend_type, offset_hi, offset_lo, nargs
    [OP_RETURN] = 0,

    // Variables
    [OP_DEFINE_GLOBAL] = 1, // global index
    [OP_GET_GLOBAL] = 1,
    [OP_SET_GLOBAL] = 1,
    [OP_GET_LOCAL] = 1, // stack slot
    [OP_SET_LOCAL] = 1,
    [OP_GET_UPVALUE] = 1, // upvalue index
    [OP_SET_UPVALUE] = 1,

    // Functions
    [OP_CLOSURE] = 1, // function index
    [OP_CLOSE_UPVALUE] = 0,

    // Objects
    [OP_CLASS] = 1,        // class name index
    [OP_GET_PROPERTY] = 1, // property name index
    [OP_SET_PROPERTY] = 1,
    [OP_GET_METHOD] = 1,
    [OP_INVOKE] = 1, // method name index
    [OP_INHERIT] = 0,

    // Memory
    [OP_POP] = 0,
    [OP_DUP] = 0,
    [OP_PRINT] = 0,

    // Pointers
    [OP_ADDR_OF] = 1, // variable index
    [OP_DEREF] = 0,
    [OP_STORE_PTR] = 0,

    // Array Operations
    [OP_ARRAY_LIT] = 1, // element count
    [OP_INDEX_GET] = 0,
    [OP_INDEX_SET] = 0,
    [OP_ARRAY_LEN] = 0,
    [OP_ENUM] = 2, // tag, count
    [OP_GET_TAG] = 0,
    [OP_GET_FIELD] = 1, // index

    // Special
    [OP_EOF] = 0,
};

/// Jump opcodes
static const bool jump_opcodes[] = {
    [OP_JUMP] = true,
    [OP_JUMP_IF_FALSE] = true,
    [OP_JUMP_IF_TRUE] = true,
    [OP_LOOP] = true,
};

const char *opcode_name(Opcode opcode)
{
    if (opcode >= sizeof(opcode_names) / sizeof(opcode_names[0])) {
        return "UNKNOWN_OPCODE";
    }
    return opcode_names[opcode] ? opcode_names[opcode] : "UNNAMED_OPCODE";
}

int opcode_operand_count(Opcode opcode)
{
    if (opcode >= sizeof(operand_counts) / sizeof(operand_counts[0])) {
        return -1; // Unknown opcode
    }
    return operand_counts[opcode];
}

bool opcode_is_jump(Opcode opcode)
{
    if (opcode >= sizeof(jump_opcodes) / sizeof(jump_opcodes[0])) {
        return false;
    }
    return jump_opcodes[opcode];
}
