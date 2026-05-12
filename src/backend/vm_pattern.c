/**
 * @file vm_pattern.c
 * @brief VM execution for pattern matching opcodes
 */

#include "backend/vm.h"
#include "backend/value.h"
#include "backend/opcode_pattern.h"
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// VM EXECUTION - PATTERN OPCODES
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Execute pattern matching opcode
 * Returns: true if execution should continue, false on error
 */
bool vm_execute_pattern_opcode(VM *vm, uint8_t instruction)
{
    switch (instruction) {
    
    // ──────────────────────────────────────────────────────────────────────────
    // Stack Manipulation
    // ──────────────────────────────────────────────────────────────────────────
    
    case OP_DUP: {
        // Duplicate top value
        if (vm->stack_top < vm->stack + 1) {
            vm_runtime_error(vm, "Stack underflow in DUP");
            return false;
        }
        Value top = vm->stack_top[-1];
        vm_push(vm, top);
        break;
    }
    
    case OP_DUP_N: {
        // Duplicate top N values
        uint8_t n = vm_read_byte(vm);
        if (vm->stack_top < vm->stack + n) {
            vm_runtime_error(vm, "Stack underflow in DUP_N");
            return false;
        }
        for (int i = n - 1; i >= 0; i--) {
            vm_push(vm, vm->stack_top[-n + i]);
        }
        break;
    }
    
    case OP_POP_N: {
        // Pop N values
        uint8_t n = vm_read_byte(vm);
        if (vm->stack_top < vm->stack + n) {
            vm_runtime_error(vm, "Stack underflow in POP_N");
            return false;
        }
        vm->stack_top -= n;
        break;
    }
    
    case OP_SWAP: {
        // Swap top two values
        if (vm->stack_top < vm->stack + 2) {
            vm_runtime_error(vm, "Stack underflow in SWAP");
            return false;
        }
        Value tmp = vm->stack_top[-1];
        vm->stack_top[-1] = vm->stack_top[-2];
        vm->stack_top[-2] = tmp;
        break;
    }
    
    // ──────────────────────────────────────────────────────────────────────────
    // Pattern Testing
    // ──────────────────────────────────────────────────────────────────────────
    
    case OP_PATTERN_EQ: {
        // Test equality for pattern matching
        Value b = vm_pop(vm);
        Value a = vm_pop(vm);
        
        bool equal = values_equal(a, b);
        vm_push(vm, BOOL_VAL(equal));
        break;
    }
    
    case OP_VARIANT_TEST: {
        // Test if value is variant with given tag
        Value tag_name = vm_pop(vm);
        Value value = vm_pop(vm);
        
        if (!IS_VARIANT(value)) {
            vm_push(vm, BOOL_VAL(false));
            break;
        }
        
        Variant *variant = AS_VARIANT(value);
        ObjString *expected_tag = AS_STRING(tag_name);
        
        bool matches = strcmp(variant->tag, expected_tag->chars) == 0;
        vm_push(vm, BOOL_VAL(matches));
        break;
    }
    
    case OP_TUPLE_ARITY: {
        // Test if tuple has expected arity
        Value expected_size = vm_pop(vm);
        Value tuple = vm_pop(vm);
        
        if (!IS_TUPLE(tuple)) {
            vm_push(vm, BOOL_VAL(false));
            break;
        }
        
        Tuple *tup = AS_TUPLE(tuple);
        int expected = AS_NUMBER(expected_size);
        
        bool matches = tup->count == expected;
        vm_push(vm, BOOL_VAL(matches));
        break;
    }
    
    case OP_TYPE_TEST: {
        // Test value type
        uint8_t expected_type = vm_read_byte(vm);
        Value value = vm_pop(vm);
        
        bool matches = false;
        switch (expected_type) {
            case 0: matches = IS_NUMBER(value); break;
            case 1: matches = IS_BOOL(value); break;
            case 2: matches = IS_STRING(value); break;
            case 3: matches = IS_TUPLE(value); break;
            case 4: matches = IS_VARIANT(value); break;
        }
        
        vm_push(vm, BOOL_VAL(matches));
        break;
    }
    
    // ──────────────────────────────────────────────────────────────────────────
    // Pattern Destructuring
    // ──────────────────────────────────────────────────────────────────────────
    
    case OP_TUPLE_EXTRACT: {
        // Extract tuple element by index
        Value index = vm_pop(vm);
        Value tuple = vm_pop(vm);
        
        if (!IS_TUPLE(tuple)) {
            vm_runtime_error(vm, "TUPLE_EXTRACT: value is not a tuple");
            return false;
        }
        
        Tuple *tup = AS_TUPLE(tuple);
        int idx = AS_NUMBER(index);
        
        if (idx < 0 || idx >= tup->count) {
            vm_runtime_error(vm, "Tuple index out of bounds: %d", idx);
            return false;
        }
        
        vm_push(vm, tup->elements[idx]);
        break;
    }
    
    case OP_VARIANT_EXTRACT: {
        // Extract variant field by index
        Value index = vm_pop(vm);
        Value variant = vm_pop(vm);
        
        if (!IS_VARIANT(variant)) {
            vm_runtime_error(vm, "VARIANT_EXTRACT: value is not a variant");
            return false;
        }
        
        Variant *var = AS_VARIANT(variant);
        int idx = AS_NUMBER(index);
        
        if (idx < 0 || idx >= var->field_count) {
            vm_runtime_error(vm, "Variant field index out of bounds: %d", idx);
            return false;
        }
        
        vm_push(vm, var->fields[idx]);
        break;
    }
    
    case OP_ARRAY_EXTRACT: {
        // Extract array element by index
        Value index = vm_pop(vm);
        Value array = vm_pop(vm);
        
        if (!IS_ARRAY(array)) {
            vm_runtime_error(vm, "ARRAY_EXTRACT: value is not an array");
            return false;
        }
        
        Array *arr = AS_ARRAY(array);
        int idx = AS_NUMBER(index);
        
        if (idx < 0 || idx >= arr->count) {
            vm_runtime_error(vm, "Array index out of bounds: %d", idx);
            return false;
        }
        
        vm_push(vm, arr->elements[idx]);
        break;
    }
    
    // ──────────────────────────────────────────────────────────────────────────
    // Optimized Matching
    // ──────────────────────────────────────────────────────────────────────────
    
    case OP_JUMP_TABLE: {
        // Jump table for integer matching
        Value value = vm_pop(vm);
        
        if (!IS_NUMBER(value)) {
            // Not a number, jump to default
            uint8_t table_size = vm_read_byte(vm);
            uint16_t default_offset = vm_read_short(vm);
            
            // Skip table entries
            vm->ip += table_size * 2;
            
            // Jump to default
            vm->ip += default_offset;
            break;
        }
        
        int n = AS_NUMBER(value);
        uint8_t table_size = vm_read_byte(vm);
        uint16_t default_offset = vm_read_short(vm);
        
        // Read jump table
        bool found = false;
        for (int i = 0; i < table_size; i++) {
            uint16_t offset = vm_read_short(vm);
            if (i == n) {
                vm->ip += offset;
                found = true;
                break;
            }
        }
        
        if (!found) {
            vm->ip += default_offset;
        }
        break;
    }
    
    default:
        vm_runtime_error(vm, "Unknown pattern opcode: 0x%02x", instruction);
        return false;
    }
    
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/**
 * Test if two values are equal (for pattern matching)
 */
bool values_equal(Value a, Value b)
{
    if (a.type != b.type) return false;
    
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_NIL: return true;
        case VAL_OBJ: {
            if (IS_STRING(a) && IS_STRING(b)) {
                return strcmp(AS_CSTRING(a), AS_CSTRING(b)) == 0;
            }
            return AS_OBJ(a) == AS_OBJ(b);
        }
        default: return false;
    }
}
