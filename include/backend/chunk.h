/**
 * @file chunk.h
 * @brief Bytecode chunk with constants and line information
 *
 * A chunk represents a compiled unit of code containing:
 * - Bytecode instructions
 * - Constant pool (literals)
 * - Line number information for debugging
 */

#ifndef NOVA_CHUNK_H
#define NOVA_CHUNK_H

#include "./opcode.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ══════════════════════════════════════════════════════════════════════════════
// VALUE SYSTEM
// ══════════════════════════════════════════════════════════════════════════════

/// Tagged union for runtime values
typedef enum {
  VAL_BOOL,
  VAL_NULL,
  VAL_NUMBER,
  VAL_OBJ,   // For strings, etc. (future)
  VAL_ARRAY, // For arrays
  VAL_ENUM,  // For enums
} ValueType;

typedef struct {
  int tag;
  int field_count;
  struct Value *fields;
} EnumObj;

typedef struct {
  int count;
  struct Value *elements;
} ArrayObj;

typedef struct Value {
  ValueType type;
  union {
    bool boolean;
    double number;
    const char *string; // For string literals
    ArrayObj *array;    // For arrays
    EnumObj *enum_val;  // For enums
  } as;
} Value;

// ══════════════════════════════════════════════════════════════════════════════
// CONSTANT POOL
// ══════════════════════════════════════════════════════════════════════════════

/// Dynamic array of constants
typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

// ══════════════════════════════════════════════════════════════════════════════
// LINE NUMBER TRACKING
// ══════════════════════════════════════════════════════════════════════════════

/// Run-length encoded line numbers
/// Each entry represents a sequence of opcodes on the same line
typedef struct {
  int capacity;
  int count;
  int *lines; // Line numbers
  int *runs;  // How many opcodes on each line
} LineArray;

// ══════════════════════════════════════════════════════════════════════════════
// BYTECODE CHUNK
// ══════════════════════════════════════════════════════════════════════════════

/// A chunk of bytecode
typedef struct {
  int capacity;
  int count;
  uint8_t *code;        // The bytecode
  ValueArray constants; // Constant pool
  LineArray lines;      // Line information
} Chunk;

// ══════════════════════════════════════════════════════════════════════════════
// CHUNK API
// ══════════════════════════════════════════════════════════════════════════════

/// Initialize an empty chunk
void chunk_init(Chunk *chunk);

/// Free chunk resources
void chunk_free(Chunk *chunk);

/// Write a byte to the chunk
void chunk_write(Chunk *chunk, uint8_t byte, int line);

/// Write an opcode to the chunk
void chunk_write_opcode(Chunk *chunk, Opcode opcode, int line);

/// Add a constant to the pool and return its index
int chunk_add_constant(Chunk *chunk, Value value);

// ══════════════════════════════════════════════════════════════════════════════
// VALUE API
// ══════════════════════════════════════════════════════════════════════════════

/// Create values
Value value_bool(bool boolean);
Value value_null(void);
Value value_number(double number);
Value value_string(const char *string);
Value value_array(int count, Value *elements);
Value value_enum(int tag, int count, Value *fields);

/// Check value types
bool value_is_bool(Value value);
bool value_is_null(Value value);
bool value_is_number(Value value);

/// Extract values
bool value_as_bool(Value value);
double value_as_number(Value value);

/// Compare values
bool value_equals(Value a, Value b);

/// Print value for debugging
void value_print(Value value);

// ══════════════════════════════════════════════════════════════════════════════
// VALUE ARRAY API
// ══════════════════════════════════════════════════════════════════════════════

/// Initialize value array
void value_array_init(ValueArray *array);

/// Free value array
void value_array_free(ValueArray *array);

/// Write value to array
void value_array_write(ValueArray *array, Value value);

// ══════════════════════════════════════════════════════════════════════════════
// LINE ARRAY API
// ══════════════════════════════════════════════════════════════════════════════

/// Initialize line array
void line_array_init(LineArray *array);

/// Free line array
void line_array_free(LineArray *array);

/// Add line information
void line_array_write(LineArray *array, int line);

/// Get line number for instruction at index
int line_array_get(const LineArray *array, int index);

/// Save chunk to file
void chunk_save(const Chunk *chunk, const char *filename);

/// Load chunk from file
bool chunk_load(Chunk *chunk, const char *filename);

#endif // NOVA_CHUNK_H
