/**
 * @file opcode.h
 * @brief Bytecode opcodes for the Nova VM
 *
 * This file defines all opcodes used by the Nova bytecode virtual machine.
 * The opcodes are designed to be simple, efficient, and extensible.
 *
 * Categories:
 * - Constants: loading literals
 * - Arithmetic: math operations
 * - Comparison: relational operations
 * - Control flow: jumps, calls
 * - Memory: load/store operations
 * - Stack: stack manipulation
 */

#ifndef NOVA_OPCODE_H
#define NOVA_OPCODE_H

#include <stdbool.h>
#include <stdint.h>

// ══════════════════════════════════════════════════════════════════════════════
// OPCODE DEFINITIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Nova bytecode opcodes
/// Each opcode is a single byte for efficiency
typedef enum {
    // ── Constants (0x00 - 0x0F) ──────────────────────────────────────────────────

    /// Load constant from constant pool: OP_CONSTANT <index>
    OP_CONSTANT = 0x00,

    /// Load null value
    OP_NULL = 0x01,

    /// Load boolean true
    OP_TRUE = 0x02,

    /// Load boolean false
    OP_FALSE = 0x03,

    // ── Arithmetic Operations (0x10 - 0x2F) ────────────────────────────────────

    /// Add two values: a + b
    OP_ADD = 0x10,

    /// Subtract: a - b
    OP_SUBTRACT = 0x11,

    /// Multiply: a * b
    OP_MULTIPLY = 0x12,

    /// Divide: a / b
    OP_DIVIDE = 0x13,

    /// Negate: -a
    OP_NEGATE = 0x14,

    /// Modulo: a % b
    OP_MODULO = 0x15,

    // ── Comparison Operations (0x30 - 0x3F) ────────────────────────────────────

    /// Equal: a == b
    OP_EQUAL = 0x30,

    /// Not equal: a != b
    OP_NOT_EQUAL = 0x31,

    /// Less than: a < b
    OP_LESS = 0x32,

    /// Less or equal: a <= b
    OP_LESS_EQUAL = 0x33,

    /// Greater than: a > b
    OP_GREATER = 0x34,

    /// Greater or equal: a >= b
    OP_GREATER_EQUAL = 0x35,

    // ── Logical Operations (0x40 - 0x4F) ───────────────────────────────────────

    /// Logical NOT: !a
    OP_NOT = 0x40,

    /// Logical AND: a && b
    OP_AND = 0x41,

    /// Logical OR: a || b
    OP_OR = 0x42,

    // ── Control Flow (0x50 - 0x6F) ─────────────────────────────────────────────

    /// Jump to offset: pc += offset
    OP_JUMP = 0x50,

    /// Jump if false: if (!condition) pc += offset
    OP_JUMP_IF_FALSE = 0x51,

    /// Jump if true: if (condition) pc += offset
    OP_JUMP_IF_TRUE = 0x52,

    /// Loop: pc -= offset
    OP_LOOP = 0x53,

    /// Call function: call function at stack[top-1] with nargs args
    OP_CALL = 0x54,

    /// Call VPU kernel: OP_VPU_CALL <backend_type> <fn_offset_16> <nargs>
    OP_VPU_CALL = 0x58,

    /// Return from function
    OP_RETURN = 0x55,

    // ── Variables & Scoping (0x70 - 0x8F) ──────────────────────────────────────

    /// Define global variable: define global at index
    OP_DEFINE_GLOBAL = 0x70,

    /// Get global variable: push globals[index]
    OP_GET_GLOBAL = 0x71,

    /// Set global variable: globals[index] = stack[top]
    OP_SET_GLOBAL = 0x72,

    /// Get local variable: push locals[index]
    OP_GET_LOCAL = 0x73,

    /// Set local variable: locals[index] = stack[top]
    OP_SET_LOCAL = 0x74,

    /// Get upvalue: push upvalues[index]
    OP_GET_UPVALUE = 0x75,

    /// Set upvalue: upvalues[index] = stack[top]
    OP_SET_UPVALUE = 0x76,

    // ── Closures & Functions (0x90 - 0x9F) ─────────────────────────────────────

    /// Create closure from function
    OP_CLOSURE = 0x90,

    /// Close upvalue
    OP_CLOSE_UPVALUE = 0x91,

    // ── Classes & Objects (0xA0 - 0xBF) ────────────────────────────────────────

    /// Create class
    OP_CLASS = 0xA0,

    /// Get property: obj.prop
    OP_GET_PROPERTY = 0xA1,

    /// Set property: obj.prop = value
    OP_SET_PROPERTY = 0xA2,

    /// Get method: obj.method
    OP_GET_METHOD = 0xA3,

    /// Invoke method: obj.method(args...)
    OP_INVOKE = 0xA4,

    /// Inherit from superclass
    OP_INHERIT = 0xA5,

    // ── Enum Operations (0xB0 - 0xBF) ──────────────────────────────────────────

    /// Create enum variant: OP_ENUM <tag> <field_count>
    OP_ENUM = 0xB0,

    /// Get enum tag: push tag of enum at stack top
    OP_GET_TAG = 0xB1,

    /// Get enum field: push enum.fields[index]
    OP_GET_FIELD = 0xB2,

    // ── Memory & Stack (0xC0 - 0xCF) ───────────────────────────────────────────

    /// Pop value from stack
    OP_POP = 0xC0,

    /// Duplicate top of stack
    OP_DUP = 0xC1,

    /// Print value (for debugging)
    OP_PRINT = 0xC2,

    // ── Pointer Operations (0xD0 - 0xDF) ───────────────────────────────────────

    /// Address-of: push address of variable
    OP_ADDR_OF = 0xD0,

    /// Dereference: load value from pointer
    OP_DEREF = 0xD1,

    /// Store through pointer: *ptr = value
    OP_STORE_PTR = 0xD2,

    // ── Array Operations (0xE0 - 0xEF) ─────────────────────────────────────────

    /// Create array from stack: array[top-count...top]
    OP_ARRAY_LIT = 0xE0,

    /// Index into array: push object[index]
    OP_INDEX_GET = 0xE1,

    /// Set array element: object[index] = value
    OP_INDEX_SET = 0xE2,

    /// Get array length: push object.count
    OP_ARRAY_LEN = 0xE3,

    // ── Unit Algebra Opcodes (0x56 - 0x57) ─────────────────────────────────────

    /// Scale a numeric value by a compile-time constant: OP_UNIT_SCALE <scale_const_index>
    /// Pops a number, multiplies by scale factor, pushes result.
    /// Used for unit conversions like `5.km` (pushes 5000.0 in SI meters).
    OP_UNIT_SCALE = 0x56,

    /// Convert between compatible units at runtime: OP_UNIT_CONVERT <from_const> <to_const>
    /// Pops a number, applies nova_dim_convert(from, to), pushes result.
    OP_UNIT_CONVERT = 0x57,

    // ── Special Opcodes (0xF0 - 0xFF) ──────────────────────────────────────────

    /// End of bytecode marker
    OP_EOF = 0xFF,

} Opcode;

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Get the name of an opcode for debugging
const char *opcode_name(Opcode opcode);

/// Get the number of operands for an opcode
/// Returns -1 for variable-length opcodes
int opcode_operand_count(Opcode opcode);

/// Check if opcode is a jump instruction
bool opcode_is_jump(Opcode opcode);

#endif // NOVA_OPCODE_H
