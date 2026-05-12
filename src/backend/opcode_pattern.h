/**
 * @file opcode_pattern.h
 * @brief Opcodes for pattern matching
 */

#ifndef NOVA_OPCODE_PATTERN_H
#define NOVA_OPCODE_PATTERN_H

#include <stdint.h>

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN MATCHING OPCODES
// ══════════════════════════════════════════════════════════════════════════════

// Stack manipulation
#define OP_DUP              0x20  // Duplicate top of stack
#define OP_DUP_N            0x21  // Duplicate N values
#define OP_POP_N            0x22  // Pop N values
#define OP_SWAP             0x23  // Swap top two values

// Pattern testing
#define OP_PATTERN_EQ       0x50  // Test equality (for literals)
#define OP_VARIANT_TEST     0x51  // Test variant tag
#define OP_TUPLE_ARITY      0x52  // Test tuple arity
#define OP_TYPE_TEST        0x53  // Test value type

// Pattern destructuring
#define OP_TUPLE_EXTRACT    0x54  // Extract tuple element by index
#define OP_VARIANT_EXTRACT  0x55  // Extract variant field by index
#define OP_ARRAY_EXTRACT    0x56  // Extract array element

// Optimized matching
#define OP_JUMP_TABLE       0x60  // Jump table for integer literals
#define OP_MATCH_INT        0x61  // Optimized integer match
#define OP_MATCH_STR        0x62  // Optimized string match

// ══════════════════════════════════════════════════════════════════════════════
// OPCODE DESCRIPTIONS
// ══════════════════════════════════════════════════════════════════════════════

/*
OP_DUP:
  Stack: [value] -> [value, value]
  Duplicates the top value on the stack

OP_PATTERN_EQ:
  Stack: [value1, value2] -> [bool]
  Tests if two values are equal (for pattern matching)

OP_VARIANT_TEST:
  Stack: [value, tag_name] -> [bool]
  Tests if value is a variant with given tag

OP_TUPLE_ARITY:
  Stack: [tuple, expected_size] -> [bool]
  Tests if tuple has expected number of elements

OP_TUPLE_EXTRACT:
  Stack: [tuple, index] -> [element]
  Extracts element at index from tuple

OP_VARIANT_EXTRACT:
  Stack: [variant, index] -> [field]
  Extracts field at index from variant

OP_JUMP_TABLE:
  Bytecode: [OP_JUMP_TABLE, table_size, default_offset, offset_0, offset_1, ...]
  Stack: [value] -> []
  Jumps to offset based on value (for integer match)
  If value not in table, jumps to default_offset
*/

#endif // NOVA_OPCODE_PATTERN_H
