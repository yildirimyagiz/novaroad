/**
 * @file chunk.c
 * @brief Implementation of bytecode chunk functions
 */

#include "../../include/backend/chunk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY MACROS
// ══════════════════════════════════════════════════════════════════════════════

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count)                                            \
    (type *) realloc(pointer, sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count) free(pointer)

// ══════════════════════════════════════════════════════════════════════════════
// VALUE FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

Value value_bool(bool boolean)
{
    return (Value) {VAL_BOOL, {.boolean = boolean}};
}

Value value_null(void)
{
    return (Value) {VAL_NULL, {.number = 0}}; // number field unused
}

Value value_number(double number)
{
    return (Value) {VAL_NUMBER, {.number = number}};
}

Value value_string(const char *string)
{
    return (Value) {VAL_OBJ, {.string = string}};
}

Value value_array(int count, Value *elements)
{
    ArrayObj *obj = malloc(sizeof(ArrayObj));
    obj->count = count;
    obj->elements = malloc(sizeof(Value) * count);
    memcpy(obj->elements, elements, sizeof(Value) * count);
    return (Value) {VAL_ARRAY, {.array = obj}};
}

Value value_enum(int tag, int count, Value *fields)
{
    EnumObj *obj = malloc(sizeof(EnumObj));
    obj->tag = tag;
    obj->field_count = count;
    if (count > 0) {
        obj->fields = malloc(sizeof(Value) * count);
        memcpy(obj->fields, fields, sizeof(Value) * count);
    } else {
        obj->fields = NULL;
    }
    return (Value) {VAL_ENUM, {.enum_val = obj}};
}

bool value_is_bool(Value value)
{
    return value.type == VAL_BOOL;
}

bool value_is_null(Value value)
{
    return value.type == VAL_NULL;
}

bool value_is_number(Value value)
{
    return value.type == VAL_NUMBER;
}

bool value_as_bool(Value value)
{
    return value.as.boolean;
}

double value_as_number(Value value)
{
    return value.as.number;
}

bool value_equals(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type) {
    case VAL_BOOL:
        return value_as_bool(a) == value_as_bool(b);
    case VAL_NULL:
        return true;
    case VAL_NUMBER:
        return value_as_number(a) == value_as_number(b);
    case VAL_ARRAY: {
        if (a.as.array->count != b.as.array->count)
            return false;
        for (int i = 0; i < a.as.array->count; i++) {
            if (!value_equals(a.as.array->elements[i], b.as.array->elements[i]))
                return false;
        }
        return true;
    }
    case VAL_ENUM: {
        if (a.as.enum_val->tag != b.as.enum_val->tag)
            return false;
        if (a.as.enum_val->field_count != b.as.enum_val->field_count)
            return false;
        for (int i = 0; i < a.as.enum_val->field_count; i++) {
            if (!value_equals(a.as.enum_val->fields[i], b.as.enum_val->fields[i]))
                return false;
        }
        return true;
    }
    default:
        return false;
    }
}

void value_print(Value value)
{
    switch (value.type) {
    case VAL_BOOL:
        printf(value_as_bool(value) ? "true" : "false");
        break;
    case VAL_NULL:
        printf("null");
        break;
    case VAL_NUMBER:
        printf("%g", value_as_number(value));
        break;
    case VAL_OBJ:
        if (value.as.string) {
            printf("%s", value.as.string);
        } else {
            printf("<null string>");
        }
        break;
        printf("]");
        break;
    case VAL_ENUM:
        printf("Variant(%d", value.as.enum_val->tag);
        if (value.as.enum_val->field_count > 0) {
            printf("(");
            for (int i = 0; i < value.as.enum_val->field_count; i++) {
                value_print(value.as.enum_val->fields[i]);
                if (i < value.as.enum_val->field_count - 1)
                    printf(", ");
            }
            printf(")");
        }
        printf(")");
        break;
    default:
        printf("<unknown>");
        break;
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// VALUE ARRAY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

void value_array_init(ValueArray *array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void value_array_free(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    value_array_init(array);
}

void value_array_write(ValueArray *array, Value value)
{
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

// ══════════════════════════════════════════════════════════════════════════════
// LINE ARRAY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

void line_array_init(LineArray *array)
{
    array->lines = NULL;
    array->runs = NULL;
    array->capacity = 0;
    array->count = 0;
}

void line_array_free(LineArray *array)
{
    FREE_ARRAY(int, array->lines, array->capacity);
    FREE_ARRAY(int, array->runs, array->capacity);
    line_array_init(array);
}

void line_array_write(LineArray *array, int line)
{
    // Run-length encoding: if last line is the same, increment run count
    if (array->count > 0 && array->lines[array->count - 1] == line) {
        array->runs[array->count - 1]++;
    } else {
        // New line, add new entry
        if (array->capacity < array->count + 1) {
            int old_capacity = array->capacity;
            array->capacity = GROW_CAPACITY(old_capacity);
            array->lines = GROW_ARRAY(int, array->lines, old_capacity, array->capacity);
            array->runs = GROW_ARRAY(int, array->runs, old_capacity, array->capacity);
        }

        array->lines[array->count] = line;
        array->runs[array->count] = 1;
        array->count++;
    }
}

int line_array_get(LineArray *array, int index)
{
    int current_index = 0;

    for (int i = 0; i < array->count; i++) {
        int run_length = array->runs[i];
        if (index < current_index + run_length) {
            return array->lines[i];
        }
        current_index += run_length;
    }

    return -1; // Should not happen if index is valid
}

// ══════════════════════════════════════════════════════════════════════════════
// CHUNK FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

void chunk_init(Chunk *chunk)
{
    chunk->code = NULL;
    chunk->capacity = 0;
    chunk->count = 0;
    value_array_init(&chunk->constants);
    line_array_init(&chunk->lines);
}

void chunk_free(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    value_array_free(&chunk->constants);
    line_array_free(&chunk->lines);
    chunk_init(chunk);
}

void chunk_write(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    line_array_write(&chunk->lines, line);
    chunk->count++;
}

void chunk_write_opcode(Chunk *chunk, Opcode opcode, int line)
{
    chunk_write(chunk, (uint8_t) opcode, line);
}

int chunk_add_constant(Chunk *chunk, Value value)
{
    value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;
}
