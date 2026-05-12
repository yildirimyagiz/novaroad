/**
 * @file vm.c
 * @brief Nova Virtual Machine implementation
 */

#include "backend/vm.h"
#include "backend/bytecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// VM STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

struct nova_vm {
  Chunk *chunk;
  uint8_t *ip; // Instruction pointer
  Value stack[STACK_MAX];
  Value *stack_top;
  Value globals[256]; // Global variables
  char *error_message;

  // Simple call stack (return addresses)
  uint8_t *call_stack[4096]; // Increased from 256 to support deeper recursion
  int call_stack_top;
};

// ══════════════════════════════════════════════════════════════════════════════
// STACK OPERATIONS
// ══════════════════════════════════════════════════════════════════════════════

static void push(nova_vm_t *vm, Value value) {
  if (vm->stack_top - vm->stack >= STACK_MAX) {
    vm->error_message = strdup("Stack overflow");
    return;
  }
  *vm->stack_top = value;
  vm->stack_top++;
}

static Value pop(nova_vm_t *vm) {
  // Note: Caller should ensure stack is not empty
  // We don't set error here because it would persist across operations
  if (vm->stack_top == vm->stack) {
    return value_null(); // Return null but don't set error
  }
  vm->stack_top--;
  return *vm->stack_top;
}

static Value peek(nova_vm_t *vm, int distance) {
  return vm->stack_top[-1 - distance];
}

// ══════════════════════════════════════════════════════════════════════════════
// BINARY OPERATIONS
// ══════════════════════════════════════════════════════════════════════════════

static void binary_op(nova_vm_t *vm, Opcode op) {
  Value b = pop(vm);
  Value a = pop(vm);

  // Special case for string concatenation with +
  if (op == OP_ADD && (a.type == VAL_OBJ || b.type == VAL_OBJ)) {
    // Convert both to strings and concatenate
    const char *left_str = (a.type == VAL_OBJ) ? a.as.string : "";
    const char *right_str = (b.type == VAL_OBJ) ? b.as.string : "";

    // Handle number to string conversion
    char left_buf[32] = {0};
    char right_buf[32] = {0};

    if (a.type == VAL_NUMBER) {
      snprintf(left_buf, sizeof(left_buf), "%g", a.as.number);
      left_str = left_buf;
    }
    if (b.type == VAL_NUMBER) {
      snprintf(right_buf, sizeof(right_buf), "%g", b.as.number);
      right_str = right_buf;
    }

    size_t len = strlen(left_str) + strlen(right_str) + 1;
    char *result_str = malloc(len);
    strcpy(result_str, left_str);
    strcat(result_str, right_str);

    // Note: value_string takes ownership of the pointer, don't free it
    Value result = value_string(result_str);
    push(vm, result);
    return;
  }

  if (!value_is_number(a) || !value_is_number(b)) {
    vm->error_message = strdup("Operands must be numbers");
    return;
  }

  double left = value_as_number(a);
  double right = value_as_number(b);
  Value result;

  switch (op) {
  case OP_ADD:
    result = value_number(left + right);
    break;
  case OP_SUBTRACT:
    result = value_number(left - right);
    break;
  case OP_MULTIPLY:
    result = value_number(left * right);
    break;
  case OP_DIVIDE:
    result = value_number(left / right);
    break;
  default:
    vm->error_message = strdup("Unknown binary operator");
    return;
  }

  push(vm, result);
}

// ══════════════════════════════════════════════════════════════════════════════
// EXECUTION LOOP
// ══════════════════════════════════════════════════════════════════════════════

static NovaInterpretResult run(nova_vm_t *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_SHORT() ((uint16_t)(READ_BYTE() << 8) | READ_BYTE())
#define BINARY_OP(op)                                                          \
  do {                                                                         \
    binary_op(vm, op);                                                         \
    if (vm->error_message)                                                     \
      return INTERPRET_RUNTIME_ERROR;                                          \
  } while (false)

  uint8_t *code_end = vm->chunk->code + vm->chunk->count;

  while (vm->ip < code_end) {
    uint8_t instruction = READ_BYTE();

    switch (instruction) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(vm, constant);
      break;
    }

    case OP_NULL:
      push(vm, value_null());
      break;
    case OP_TRUE:
      push(vm, value_bool(true));
      break;
    case OP_FALSE:
      push(vm, value_bool(false));
      break;

    case OP_ADD:
      BINARY_OP(OP_ADD);
      break;
    case OP_SUBTRACT:
      BINARY_OP(OP_SUBTRACT);
      break;
    case OP_MULTIPLY:
      BINARY_OP(OP_MULTIPLY);
      break;
    case OP_DIVIDE:
      BINARY_OP(OP_DIVIDE);
      break;

    // Comparison operators
    case OP_GREATER: {
      Value b = pop(vm);
      Value a = pop(vm);
      push(vm, value_bool(value_as_number(a) > value_as_number(b)));
      break;
    }
    case OP_LESS: {
      Value b = pop(vm);
      Value a = pop(vm);
      push(vm, value_bool(value_as_number(a) < value_as_number(b)));
      break;
    }
    case OP_GREATER_EQUAL: {
      Value b = pop(vm);
      Value a = pop(vm);
      push(vm, value_bool(value_as_number(a) >= value_as_number(b)));
      break;
    }
    case OP_LESS_EQUAL: {
      Value b = pop(vm);
      Value a = pop(vm);
      push(vm, value_bool(value_as_number(a) <= value_as_number(b)));
      break;
    }
    case OP_EQUAL: {
      Value b = pop(vm);
      Value a = pop(vm);
      push(vm, value_bool(value_as_number(a) == value_as_number(b)));
      break;
    }
    case OP_NOT_EQUAL: {
      Value b = pop(vm);
      Value a = pop(vm);
      push(vm, value_bool(value_as_number(a) != value_as_number(b)));
      break;
    }

    case OP_NEGATE:
      if (!value_is_number(peek(vm, 0))) {
        vm->error_message = strdup("Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, value_number(-value_as_number(pop(vm))));
      break;

    case OP_DEFINE_GLOBAL: {
      uint8_t index = READ_BYTE();
      vm->globals[index] = pop(vm);
      break;
    }

    case OP_GET_GLOBAL: {
      uint8_t index = READ_BYTE();
      push(vm, vm->globals[index]);
      break;
    }

    case OP_SET_GLOBAL: {
      uint8_t index = READ_BYTE();
      vm->globals[index] = peek(vm, 0);
      break;
    }

    case OP_GET_LOCAL: {
      uint8_t index = READ_BYTE();
      push(vm, vm->stack[index]);
      break;
    }

    case OP_SET_LOCAL: {
      uint8_t index = READ_BYTE();
      vm->stack[index] = peek(vm, 0);
      break;
    }

    case OP_POP: {
      pop(vm);
      break;
    }

    case OP_DUP: {
      // Duplicate top of stack
      if (vm->stack_top > vm->stack) {
        Value top = *(vm->stack_top - 1);
        push(vm, top);
      } else {
        vm->error_message = strdup("Stack underflow in DUP");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_PRINT: {
      value_print(pop(vm));
      printf("\n");
      push(vm, value_null());
      break;
    }

    case OP_ARRAY_LIT: {
      uint8_t count = READ_BYTE();
      Value *elements = vm->stack_top - count;
      Value array = value_array(count, elements);
      vm->stack_top -= count; // Pop elements
      push(vm, array);
      break;
    }

    case OP_INDEX_GET: {
      Value index_val = pop(vm);
      Value object = pop(vm);

      if (object.type != VAL_ARRAY) {
        vm->error_message = strdup("Only arrays can be indexed");
        return INTERPRET_RUNTIME_ERROR;
      }

      if (index_val.type != VAL_NUMBER) {
        vm->error_message = strdup("Array index must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }

      int index = (int)value_as_number(index_val);
      if (index < 0 || index >= object.as.array->count) {
        vm->error_message = strdup("Array index out of bounds");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(vm, object.as.array->elements[index]);
      break;
    }

    case OP_INDEX_SET: {
      Value value = pop(vm);
      Value index_val = pop(vm);
      Value object = pop(vm);

      if (object.type != VAL_ARRAY) {
        vm->error_message = strdup("Only arrays can be indexed");
        return INTERPRET_RUNTIME_ERROR;
      }

      if (index_val.type != VAL_NUMBER) {
        vm->error_message = strdup("Array index must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }

      int index = (int)value_as_number(index_val);
      if (index < 0 || index >= object.as.array->count) {
        vm->error_message = strdup("Array index out of bounds");
        return INTERPRET_RUNTIME_ERROR;
      }

      object.as.array->elements[index] = value;
      push(vm, value); // Standard assignment returns the value
      break;
    }

    case OP_ARRAY_LEN: {
      Value object = pop(vm);
      if (object.type != VAL_ARRAY) {
        vm->error_message = strdup("Operand must be an array");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, value_number((double)object.as.array->count));
      break;
    }

    case OP_UNIT_SCALE: {
      /* OP_UNIT_SCALE <scale_const_index>
       * Pops a number, multiplies by scale factor, pushes result.
       * Used for: unit literals with non-SI scale (km, cm, g, etc.)
       * and for `in` unit conversion (inverse scale applied). */
      uint8_t scale_idx = READ_BYTE();
      Value scale_val = vm->chunk->constants.values[scale_idx];
      Value operand = pop(vm);
      if (operand.type != VAL_NUMBER || scale_val.type != VAL_NUMBER) {
        vm->error_message = strdup("OP_UNIT_SCALE: operands must be numbers");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, value_number(operand.as.number * scale_val.as.number));
      break;
    }

    case OP_UNIT_CONVERT: {
      /* OP_UNIT_CONVERT <from_const_idx> <to_const_idx>
       * Both constants hold scale factors (doubles).
       * Converts: value * (from_scale / to_scale).
       * This opcode is reserved for future runtime dispatch; currently
       * the compiler resolves conversions at compile time via OP_UNIT_SCALE. */
      uint8_t from_idx = READ_BYTE();
      uint8_t to_idx = READ_BYTE();
      Value from_val = vm->chunk->constants.values[from_idx];
      Value to_val = vm->chunk->constants.values[to_idx];
      Value operand = pop(vm);
      if (operand.type != VAL_NUMBER || from_val.type != VAL_NUMBER ||
          to_val.type != VAL_NUMBER || to_val.as.number == 0.0) {
        vm->error_message = strdup("OP_UNIT_CONVERT: invalid operands");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, value_number(operand.as.number *
                            (from_val.as.number / to_val.as.number)));
      break;
    }

    case OP_ENUM: {
      uint8_t tag = READ_BYTE();
      uint8_t count = READ_BYTE();
      Value *fields = NULL;
      if (count > 0) {
        fields = malloc(sizeof(Value) * count);
        for (int i = count - 1; i >= 0; i--) {
          fields[i] = pop(vm);
        }
      }
      Value enum_val = value_enum(tag, (int)count, fields);
      push(vm, enum_val);
      if (fields)
        free(fields);
      break;
    }

    case OP_GET_TAG: {
      Value object = peek(vm, 0);
      if (object.type != VAL_ENUM) {
        vm->error_message = strdup("Expected enum for tag access");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, value_number((double)object.as.enum_val->tag));
      break;
    }

    case OP_GET_FIELD: {
      uint8_t index = READ_BYTE();
      Value object = peek(vm, 0);
      if (object.type != VAL_ENUM) {
        vm->error_message = strdup("Expected enum for field access");
        return INTERPRET_RUNTIME_ERROR;
      }
      if (index >= object.as.enum_val->field_count) {
        vm->error_message = strdup("Enum field index out of bounds");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(vm, object.as.enum_val->fields[index]);
      break;
    }

    // Control flow
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      vm->ip += offset;
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      Value condition = pop(vm);
      if (!value_as_bool(condition)) {
        vm->ip += offset;
      }
      break;
    }

    case OP_JUMP_IF_TRUE: {
      uint16_t offset = READ_SHORT();
      Value condition = pop(vm);
      if (value_as_bool(condition)) {
        vm->ip += offset;
      }
      break;
    }

    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      vm->ip -= offset;
      break;
    }

    case OP_CALL: {
      uint16_t fn_offset = READ_SHORT();
      uint8_t arg_count = READ_BYTE();
      (void)arg_count; // In this VM, locals are absolute, so we don't adjust
                       // stack base

      if (vm->call_stack_top >= 4096) {
        vm->error_message = strdup("Call stack overflow");
        return INTERPRET_RUNTIME_ERROR;
      }
      vm->call_stack[vm->call_stack_top++] = vm->ip;
      vm->ip = vm->chunk->code + fn_offset;
      break;
    }

    case OP_VPU_CALL: {
      // OP_VPU_CALL <backend_type> <fn_offset_16> <nargs>
      uint8_t backend_type = READ_BYTE();
      uint16_t fn_offset = READ_SHORT();
      uint8_t arg_count = READ_BYTE();
      (void)arg_count;

      printf("🚀 VM: VPU Dispatch to Backend %d (Offset %d)\n", backend_type,
             fn_offset);

      // In a real implementation, we would use the execution fabric to
      // dispatch the kernel. For now, we simulate by doing a normal call
      // but logging the VPU target.

      if (vm->call_stack_top >= 4096) {
        vm->error_message = strdup("Call stack overflow");
        return INTERPRET_RUNTIME_ERROR;
      }
      vm->call_stack[vm->call_stack_top++] = vm->ip;
      vm->ip = vm->chunk->code + fn_offset;
      break;
    }

    case OP_RETURN: {
      // Pop return address if available
      if (vm->call_stack_top > 0) {
        vm->ip = vm->call_stack[--vm->call_stack_top];
        break;
      }

      // Exit interpreter if no more calls
      return INTERPRET_OK;
    }

    default:
      vm->error_message = strdup("Unknown opcode");
      return INTERPRET_RUNTIME_ERROR;
    }

    // Check for errors
    if (vm->error_message) {
      return INTERPRET_RUNTIME_ERROR;
    }
  }

  return INTERPRET_OK;

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef BINARY_OP
}

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ══════════════════════════════════════════════════════════════════════════════

nova_vm_t *nova_vm_create(void) {
  nova_vm_t *vm = (nova_vm_t *)malloc(sizeof(nova_vm_t));
  if (!vm)
    return NULL;

  vm->chunk = NULL;
  vm->ip = NULL;
  vm->stack_top = vm->stack;
  vm->error_message = NULL;
  vm->call_stack_top = 0;

  // Initialize globals to null
  for (int i = 0; i < 256; i++) {
    vm->globals[i] = value_null();
  }

  return vm;
}

NovaInterpretResult nova_vm_interpret(nova_vm_t *vm, Chunk *chunk) {
  vm->chunk = chunk;
  vm->ip = chunk->code;
  vm->stack_top = vm->stack;

  vm->call_stack_top = 0;

  // Free previous error message
  if (vm->error_message) {
    free(vm->error_message);
    vm->error_message = NULL;
  }

  return run(vm);
}

const char *nova_vm_get_error(nova_vm_t *vm) {
  return vm->error_message ? vm->error_message : "No error";
}

void nova_vm_destroy(nova_vm_t *vm) {
  if (vm->error_message) {
    free(vm->error_message);
  }
  free(vm);
}
