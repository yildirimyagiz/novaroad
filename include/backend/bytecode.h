/**
 * @file bytecode.h
 * @brief Bytecode encoding and decoding utilities
 *
 * Functions for reading and writing bytecode instructions from/to chunks.
 * Handles variable-length encoding and provides a bytecode reader.
 */

#ifndef NOVA_BYTECODE_H
#define NOVA_BYTECODE_H

#include "chunk.h"
#include <stdbool.h>
#include <stdint.h>

// ══════════════════════════════════════════════════════════════════════════════
// BYTECODE READER
// ══════════════════════════════════════════════════════════════════════════════

/// Bytecode reader for interpreting chunks
typedef struct {
  const Chunk *chunk; // The chunk being read
  int ip;             // Instruction pointer (current position)
} BytecodeReader;

// ══════════════════════════════════════════════════════════════════════════════
// BYTECODE API
// ══════════════════════════════════════════════════════════════════════════════

/// Initialize bytecode reader
void bytecode_reader_init(BytecodeReader *reader, const Chunk *chunk);

/// Read next byte
uint8_t bytecode_read_byte(BytecodeReader *reader);

/// Read next opcode
Opcode bytecode_read_opcode(BytecodeReader *reader);

/// Read constant index (1 byte)
uint8_t bytecode_read_constant_index(BytecodeReader *reader);

/// Read jump offset (2 bytes, signed)
int16_t bytecode_read_jump_offset(BytecodeReader *reader);

/// Peek at next byte without advancing
uint8_t bytecode_peek_byte(const BytecodeReader *reader);

/// Check if reader is at end
bool bytecode_is_at_end(const BytecodeReader *reader);

/// Get current line number
int bytecode_current_line(const BytecodeReader *reader);

// ══════════════════════════════════════════════════════════════════════════════
// ENCODING FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Write constant index to chunk
void bytecode_write_constant_index(Chunk *chunk, uint8_t index, int line);

/// Write jump offset to chunk
void bytecode_write_jump_offset(Chunk *chunk, int16_t offset, int line);

// ══════════════════════════════════════════════════════════════════════════════
// DISASSEMBLY
// ══════════════════════════════════════════════════════════════════════════════

/// Disassemble entire chunk (for debugging)
void bytecode_disassemble_chunk(const Chunk *chunk, const char *name);

/// Disassemble single instruction
int bytecode_disassemble_instruction(const Chunk *chunk, int offset);

#endif // NOVA_BYTECODE_H
