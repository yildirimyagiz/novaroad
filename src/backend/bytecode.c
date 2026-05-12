/**
 * @file bytecode.c
 * @brief Implementation of bytecode reading/writing utilities
 */

#include "backend/bytecode.h"
#include "backend/opcode.h"
#include <stdio.h>

// ══════════════════════════════════════════════════════════════════════════════
// BYTECODE READER
// ══════════════════════════════════════════════════════════════════════════════

void bytecode_reader_init(BytecodeReader *reader, const Chunk *chunk)
{
    reader->chunk = chunk;
    reader->ip = 0;
}

uint8_t bytecode_read_byte(BytecodeReader *reader)
{
    return reader->chunk->code[reader->ip++];
}

Opcode bytecode_read_opcode(BytecodeReader *reader)
{
    return (Opcode) bytecode_read_byte(reader);
}

uint8_t bytecode_read_constant_index(BytecodeReader *reader)
{
    return bytecode_read_byte(reader);
}

int16_t bytecode_read_jump_offset(BytecodeReader *reader)
{
    uint8_t high = bytecode_read_byte(reader);
    uint8_t low = bytecode_read_byte(reader);
    return (int16_t) ((high << 8) | low);
}

uint8_t bytecode_peek_byte(const BytecodeReader *reader)
{
    return reader->chunk->code[reader->ip];
}

bool bytecode_is_at_end(const BytecodeReader *reader)
{
    return reader->ip >= reader->chunk->count;
}

int bytecode_current_line(const BytecodeReader *reader)
{
    return line_array_get(&reader->chunk->lines, reader->ip - 1);
}

// ══════════════════════════════════════════════════════════════════════════════
// ENCODING FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

void bytecode_write_constant_index(Chunk *chunk, uint8_t index, int line)
{
    chunk_write(chunk, index, line);
}

void bytecode_write_jump_offset(Chunk *chunk, int16_t offset, int line)
{
    chunk_write(chunk, (uint8_t) ((offset >> 8) & 0xFF), line);
    chunk_write(chunk, (uint8_t) (offset & 0xFF), line);
}

// ══════════════════════════════════════════════════════════════════════════════
// DISASSEMBLY
// ══════════════════════════════════════════════════════════════════════════════

void bytecode_disassemble_chunk(const Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = bytecode_disassemble_instruction(chunk, offset);
    }
}

int bytecode_disassemble_instruction(const Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    // Print line number
    int line = line_array_get(&chunk->lines, offset);
    if (offset > 0 && line == line_array_get(&chunk->lines, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", line);
    }

    uint8_t instruction = chunk->code[offset];
    Opcode opcode = (Opcode) instruction;

    switch (opcode) {
    case OP_CONSTANT: {
        uint8_t constant_index = chunk->code[offset + 1];
        printf("%-16s %4d '", opcode_name(opcode), constant_index);
        value_print(chunk->constants.values[constant_index]);
        printf("'\n");
        return offset + 2;
    }

    case OP_NULL:
    case OP_TRUE:
    case OP_FALSE:
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_NEGATE:
    case OP_MODULO:
    case OP_EQUAL:
    case OP_NOT_EQUAL:
    case OP_LESS:
    case OP_LESS_EQUAL:
    case OP_GREATER:
    case OP_GREATER_EQUAL:
    case OP_NOT:
    case OP_AND:
    case OP_OR:
    case OP_RETURN:
    case OP_POP:
    case OP_DUP:
    case OP_PRINT:
        printf("%s\n", opcode_name(opcode));
        return offset + 1;

    case OP_JUMP:
    case OP_JUMP_IF_FALSE:
    case OP_JUMP_IF_TRUE: {
        uint16_t jump_offset = (uint16_t) (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
        printf("%-16s %4d -> %d\n", opcode_name(opcode), jump_offset,
               offset + 3 + (int16_t) jump_offset);
        return offset + 3;
    }

    case OP_LOOP: {
        uint16_t jump_offset = (uint16_t) (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
        // OP_LOOP jumps backward, so subtract the offset
        printf("%-16s %4d -> %d\n", opcode_name(opcode), jump_offset,
               offset + 3 - (int16_t) jump_offset);
        return offset + 3;
    }

    case OP_CALL: {
        uint16_t fn_offset = (uint16_t) (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
        uint8_t arg_count = chunk->code[offset + 3];
        printf("%-16s %4d (%d args)\n", opcode_name(opcode), fn_offset, arg_count);
        return offset + 4;
    }

    case OP_VPU_CALL: {
        uint8_t backend_type = chunk->code[offset + 1];
        uint16_t fn_offset = (uint16_t) (chunk->code[offset + 2] << 8) | chunk->code[offset + 3];
        uint8_t arg_count = chunk->code[offset + 4];
        printf("%-16s %4d offset=%d (%d args)\n", opcode_name(opcode), backend_type, fn_offset,
               arg_count);
        return offset + 5;
    }

    case OP_ARRAY_LIT: {
        uint8_t count = chunk->code[offset + 1];
        printf("%-16s %4d\n", opcode_name(opcode), count);
        return offset + 2;
    }

    case OP_INDEX_GET:
    case OP_INDEX_SET:
    case OP_ARRAY_LEN:
        printf("%s\n", opcode_name(opcode));
        return offset + 1;

    case OP_DEFINE_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
    case OP_GET_LOCAL:
    case OP_SET_LOCAL:
    case OP_GET_UPVALUE:
    case OP_SET_UPVALUE:
    case OP_GET_PROPERTY:
    case OP_SET_PROPERTY:
    case OP_GET_METHOD:
    case OP_INVOKE:
    case OP_GET_FIELD: {
        uint8_t index = chunk->code[offset + 1];
        printf("%-16s %4d\n", opcode_name(opcode), index);
        return offset + 2;
    }

    case OP_GET_TAG:
        printf("%s\n", opcode_name(opcode));
        return offset + 1;

    case OP_ENUM: {
        uint8_t tag = chunk->code[offset + 1];
        uint8_t count = chunk->code[offset + 2];
        printf("%-16s tag=%d count=%d\n", opcode_name(opcode), tag, count);
        return offset + 3;
    }

    case OP_CLOSURE: {
        uint8_t function_index = chunk->code[offset + 1];
        printf("%-16s %4d\n", opcode_name(opcode), function_index);
        // TODO: Handle upvalue information
        return offset + 2;
    }

    case OP_CLASS: {
        uint8_t name_index = chunk->code[offset + 1];
        printf("%-16s %4d '", opcode_name(opcode), name_index);
        value_print(chunk->constants.values[name_index]);
        printf("'\n");
        return offset + 2;
    }

    case OP_EOF:
        printf("OP_EOF\n");
        return offset + 1;

    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
