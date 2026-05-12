/**
 * @file codegen.c
 * @brief Code generation from AST to bytecode
 */

#include "compiler/codegen.h"
#include "backend/chunk.h"
#include "compiler/ast.h"
#include "compiler/dimensions.h"
#include "compiler/physics_constants.h"
#include "compiler/semantic.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Compile-time constant evaluation helpers ────────────────────────────────
 *
 * These helpers allow the codegen to fold constant unit-literal arithmetic
 * at compile time instead of emitting runtime opcodes.
 *
 * Example:
 *   5.0.km * 2.0  →  OP_CONSTANT(10000.0)   (instead of 4 opcodes)
 *   3.0.kg + 2.0.kg → OP_CONSTANT(5.0)      (same unit, fold directly)
 */

/**
 * expr_const_value: Try to evaluate an expression to a compile-time double.
 * Returns true and sets *out if the expression is a compile-time constant.
 * Handles: EXPR_LIT_FLOAT, EXPR_LIT_INT, EXPR_UNIT_LITERAL (with scale).
 */
static bool expr_const_value(const nova_expr_t *expr, double *out) {
  if (!expr || !out)
    return false;

  switch (expr->kind) {
  case EXPR_FLOAT:
    *out = expr->data.lit_float;
    return true;

  case EXPR_INT:
    *out = (double)expr->data.lit_int;
    return true;

  case EXPR_UNIT_LITERAL: {
    double raw = expr->data.unit_literal.value;
    const char *unit = expr->data.unit_literal.unit;
    if (unit && *unit) {
      nova_dimension_t *dim = nova_dim_parse(unit);
      if (dim) {
        double scale = nova_dim_get_scale(dim);
        nova_dim_destroy(dim);
        *out = raw * scale; /* SI-normalised value */
        return true;
      }
    }
    *out = raw;
    return true;
  }

  default:
    return false;
  }
}

/**
 * try_fold_binary: Try to fold a binary expression with two compile-time
 * operands. Returns true and sets *result if both sides are constants and op is
 * arithmetic.
 */
static bool try_fold_binary(const nova_expr_t *expr, double *result) {
  if (!expr || expr->kind != EXPR_BINARY)
    return false;

  const char *op = expr->data.binary.op;
  double lv, rv;

  if (!expr_const_value(expr->data.binary.left, &lv))
    return false;
  if (!expr_const_value(expr->data.binary.right, &rv))
    return false;

  /* Arithmetic ops that can be folded */
  if (strcmp(op, "+") == 0) {
    *result = lv + rv;
    return true;
  }
  if (strcmp(op, "-") == 0) {
    *result = lv - rv;
    return true;
  }
  if (strcmp(op, "*") == 0) {
    *result = lv * rv;
    return true;
  }
  if (strcmp(op, "/") == 0) {
    if (rv == 0.0)
      return false; /* avoid divide by zero */
    *result = lv / rv;
    return true;
  }

  return false; /* comparisons, 'in', etc. — not foldable here */
}

// Symbol struct forward declaration (defined in semantic.h)

// Forward declarations
static bool generate_expression(nova_codegen_t *codegen, nova_expr_t *expr);

// ══════════════════════════════════════════════════════════════════════════════
// CODE GENERATOR STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

// Simple function table
typedef struct {
  char *name;
  int bytecode_offset; // Where function starts in bytecode
  int param_count;
  int target_vpu; // 0 for default, or BackendType
} function_entry_t;

typedef struct {
  char *name;
  int field_count;
} variant_entry_t;

typedef struct {
  char *name;
  variant_entry_t *variants;
  int variant_count;
} enum_entry_t;

typedef struct {
  char *name;
  char **field_names;
  int field_count;
} struct_entry_t;

// Loop context for break/continue
typedef struct loop_context {
  int loop_start;   // Offset where loop condition starts
  int *break_jumps; // Array of break jump locations to patch
  int break_count;
  int break_capacity;
  int *continue_jumps; // Array of continue jump locations to patch
  int continue_count;
  int continue_capacity;
  struct loop_context *enclosing; // For nested loops
} loop_context_t;

struct nova_codegen {
  Chunk *chunk;
  nova_semantic_t *semantic;
  char *error_message;

  // Local variable tracking
  struct nova_symbol *locals;
  int next_local_index;
  int scope_depth;

  // Scope index stack
  int local_index_stack[64];

  // Function table
  function_entry_t *functions;
  int function_count;
  int function_capacity;

  // Loop context stack (for break/continue)
  loop_context_t *current_loop;

  // Enum table
  enum_entry_t *enums;
  int enum_count;
  int enum_capacity;

  // Struct table
  struct_entry_t *structs;
  int struct_count;
  int struct_capacity;
};

// ══════════════════════════════════════════════════════════════════════════════
// LOOP CONTEXT HELPERS
// ══════════════════════════════════════════════════════════════════════════════

static loop_context_t *loop_context_create(int loop_start,
                                           loop_context_t *enclosing) {
  loop_context_t *ctx = malloc(sizeof(loop_context_t));
  ctx->loop_start = loop_start;
  ctx->break_jumps = NULL;
  ctx->break_count = 0;
  ctx->break_capacity = 0;
  ctx->continue_jumps = NULL;
  ctx->continue_count = 0;
  ctx->continue_capacity = 0;
  ctx->enclosing = enclosing;
  return ctx;
}

static void loop_context_add_break(loop_context_t *ctx, int offset) {
  if (ctx->break_count >= ctx->break_capacity) {
    ctx->break_capacity =
        ctx->break_capacity == 0 ? 4 : ctx->break_capacity * 2;
    ctx->break_jumps =
        realloc(ctx->break_jumps, sizeof(int) * ctx->break_capacity);
  }
  ctx->break_jumps[ctx->break_count++] = offset;
}

static void loop_context_add_continue(loop_context_t *ctx, int offset) {
  if (ctx->continue_count >= ctx->continue_capacity) {
    ctx->continue_capacity =
        ctx->continue_capacity == 0 ? 4 : ctx->continue_capacity * 2;
    ctx->continue_jumps =
        realloc(ctx->continue_jumps, sizeof(int) * ctx->continue_capacity);
  }
  ctx->continue_jumps[ctx->continue_count++] = offset;
}

static void codegen_register_enum(nova_codegen_t *codegen, nova_stmt_t *stmt) {
  if (codegen->enum_count >= codegen->enum_capacity) {
    codegen->enum_capacity =
        codegen->enum_capacity == 0 ? 4 : codegen->enum_capacity * 2;
    codegen->enums =
        realloc(codegen->enums, sizeof(enum_entry_t) * codegen->enum_capacity);
  }
  enum_entry_t *e = &codegen->enums[codegen->enum_count++];
  e->name = strdup(stmt->data.enum_decl.name);
  e->variant_count = (int)stmt->data.enum_decl.variant_count;
  e->variants = malloc(sizeof(variant_entry_t) * e->variant_count);
  for (int i = 0; i < e->variant_count; i++) {
    e->variants[i].name = strdup(stmt->data.enum_decl.variants[i].name);
    e->variants[i].field_count =
        (int)stmt->data.enum_decl.variants[i].field_count;
  }
}

static void codegen_register_struct(nova_codegen_t *codegen,
                                    nova_stmt_t *stmt) {
  if (codegen->struct_count >= codegen->struct_capacity) {
    codegen->struct_capacity =
        codegen->struct_capacity == 0 ? 4 : codegen->struct_capacity * 2;
    codegen->structs = realloc(codegen->structs, sizeof(struct_entry_t) *
                                                     codegen->struct_capacity);
  }
  struct_entry_t *s = &codegen->structs[codegen->struct_count++];
  s->name = strdup(stmt->data.struct_decl->name);
  s->field_count = (int)stmt->data.struct_decl->field_count;
  s->field_names = malloc(sizeof(char *) * s->field_count);
  for (int i = 0; i < s->field_count; i++) {
    s->field_names[i] = strdup(stmt->data.struct_decl->fields[i].name);
  }
}

static bool lookup_variant_tag(nova_codegen_t *codegen,
                               const char *variant_name, int *tag,
                               int *field_count) {
  // Hardcoded variants for Result and Option (used in bootstrap)
  if (strcmp(variant_name, "Ok") == 0) {
    *tag = 0;
    *field_count = 1;
    return true;
  }
  if (strcmp(variant_name, "Err") == 0) {
    *tag = 1;
    *field_count = 1;
    return true;
  }
  if (strcmp(variant_name, "Some") == 0) {
    *tag = 0;
    *field_count = 1;
    return true;
  }
  if (strcmp(variant_name, "None") == 0) {
    *tag = 1;
    *field_count = 0;
    return true;
  }

  for (int i = 0; i < codegen->enum_count; i++) {
    for (int j = 0; j < codegen->enums[i].variant_count; j++) {
      if (strcmp(codegen->enums[i].variants[j].name, variant_name) == 0) {
        *tag = j;
        *field_count = codegen->enums[i].variants[j].field_count;
        return true;
      }
    }
  }
  // fprintf(stderr, "DEBUG: lookup_variant_tag failed for '%s'\n",
  // variant_name);
  return false;
}

static bool generate_pattern_check(nova_codegen_t *codegen,
                                   nova_pattern_t *pat) {
  if (!pat)
    return false;

  switch (pat->kind) {
  case PATTERN_ANY:
    // Always matches. Pop the duplicated target and push true.
    chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
    chunk_write_opcode(codegen->chunk, OP_TRUE, pat->span.line);
    return true;

  case PATTERN_LITERAL:
    // Target is duplicated on stack.
    // Generate literal and compare.
    if (!generate_expression(codegen, pat->data.literal))
      return false;
    chunk_write_opcode(codegen->chunk, OP_EQUAL, pat->span.line);
    return true;

  case PATTERN_IDENT: {
    // Check if this is a known enum variant name
    const char *ident = pat->data.ident;
    int tag, field_count;
    if (ident && lookup_variant_tag(codegen, ident, &tag, &field_count)) {
      // The dup'd target is on stack.
      // OP_GET_TAG peeks (doesn't pop), so we get:
      // Stack: [..., target, dup_target]
      chunk_write_opcode(codegen->chunk, OP_GET_TAG, pat->span.line);
      // Stack: [..., target, dup_target, tag]
      // Now we need to compare tag with expected and clean up dup_target.
      // Push expected tag:
      chunk_write_opcode(codegen->chunk, OP_CONSTANT, pat->span.line);
      int constant_idx =
          chunk_add_constant(codegen->chunk, value_number((double)tag));
      chunk_write(codegen->chunk, (uint8_t)constant_idx, pat->span.line);
      // Stack: [..., target, dup_target, tag, expected_tag]
      chunk_write_opcode(codegen->chunk, OP_EQUAL, pat->span.line);
      // Stack: [..., target, dup_target, bool]
      // Now we need to get rid of dup_target. Swap top two and pop:
      // We don't have a SWAP opcode, so use a different approach:
      // Store the bool result in a temp via jump trick.
      // Actually, simplest: emit OP_JUMP_IF_FALSE to handle both cases.
      chunk_write_opcode(codegen->chunk, OP_JUMP_IF_FALSE, pat->span.line);
      int false_jump = codegen->chunk->count;
      chunk_write(codegen->chunk, 0xff, pat->span.line);
      chunk_write(codegen->chunk, 0xff, pat->span.line);

      // True path: pop dup_target, push true
      chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
      chunk_write_opcode(codegen->chunk, OP_TRUE, pat->span.line);
      chunk_write_opcode(codegen->chunk, OP_JUMP, pat->span.line);
      int exit_jump = codegen->chunk->count;
      chunk_write(codegen->chunk, 0xff, pat->span.line);
      chunk_write(codegen->chunk, 0xff, pat->span.line);

      // False path: pop dup_target, push false
      int false_target = codegen->chunk->count;
      codegen->chunk->code[false_jump] =
          (uint8_t)((false_target - false_jump - 2) >> 8);
      codegen->chunk->code[false_jump + 1] =
          (uint8_t)((false_target - false_jump - 2) & 0xff);
      chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
      chunk_write_opcode(codegen->chunk, OP_FALSE, pat->span.line);

      // Exit
      int exit_target = codegen->chunk->count;
      codegen->chunk->code[exit_jump] =
          (uint8_t)((exit_target - exit_jump - 2) >> 8);
      codegen->chunk->code[exit_jump + 1] =
          (uint8_t)((exit_target - exit_jump - 2) & 0xff);
    } else if (ident && ident[0] >= 'A' && ident[0] <= 'Z') {
      // Unknown uppercase identifier - likely an unregistered variant
      // Treat as no-match
      chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
      chunk_write_opcode(codegen->chunk, OP_FALSE, pat->span.line);
    } else {
      // Lowercase - treat as binding (matches anything)
      chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
      chunk_write_opcode(codegen->chunk, OP_TRUE, pat->span.line);
    }
    return true;
  }

  case PATTERN_ENUM: {
    int tag, expected_fields;
    if (!lookup_variant_tag(codegen, pat->data.variant.name, &tag,
                            &expected_fields)) {
      // Unknown variant - pop dup and push false
      chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
      chunk_write_opcode(codegen->chunk, OP_FALSE, pat->span.line);
      return true;
    }

    // Same approach as PATTERN_IDENT enum case
    chunk_write_opcode(codegen->chunk, OP_GET_TAG, pat->span.line);
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, pat->span.line);
    int constant =
        chunk_add_constant(codegen->chunk, value_number((double)tag));
    chunk_write(codegen->chunk, (uint8_t)constant, pat->span.line);
    chunk_write_opcode(codegen->chunk, OP_EQUAL, pat->span.line);
    // Stack: [..., target, dup_target, bool]
    chunk_write_opcode(codegen->chunk, OP_JUMP_IF_FALSE, pat->span.line);
    int false_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, pat->span.line);
    chunk_write(codegen->chunk, 0xff, pat->span.line);

    chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
    chunk_write_opcode(codegen->chunk, OP_TRUE, pat->span.line);
    chunk_write_opcode(codegen->chunk, OP_JUMP, pat->span.line);
    int exit_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, pat->span.line);
    chunk_write(codegen->chunk, 0xff, pat->span.line);

    int false_target = codegen->chunk->count;
    codegen->chunk->code[false_jump] =
        (uint8_t)((false_target - false_jump - 2) >> 8);
    codegen->chunk->code[false_jump + 1] =
        (uint8_t)((false_target - false_jump - 2) & 0xff);
    chunk_write_opcode(codegen->chunk, OP_POP, pat->span.line);
    chunk_write_opcode(codegen->chunk, OP_FALSE, pat->span.line);

    int exit_target = codegen->chunk->count;
    codegen->chunk->code[exit_jump] =
        (uint8_t)((exit_target - exit_jump - 2) >> 8);
    codegen->chunk->code[exit_jump + 1] =
        (uint8_t)((exit_target - exit_jump - 2) & 0xff);

    return true;
  }
  }
  return false;
}

static void loop_context_destroy(loop_context_t *ctx) {
  if (!ctx)
    return;
  free(ctx->break_jumps);
  free(ctx->continue_jumps);
  free(ctx);
}

// ══════════════════════════════════════════════════════════════════════════════
// CODE GENERATION
// ══════════════════════════════════════════════════════════════════════════════

static nova_symbol_t *codegen_lookup_variable(nova_codegen_t *codegen,
                                              const char *name) {
  // 1. Check our own local table
  nova_symbol_t *curr = codegen->locals;
  while (curr) {
    if (strcmp(curr->name, name) == 0)
      return curr;
    curr = curr->next;
  }

  // 2. Fall back to global semantic table
  return nova_semantic_lookup_variable(codegen->semantic, name);
}

static int codegen_add_local(nova_codegen_t *codegen, const char *name) {
  nova_symbol_t *sym = malloc(sizeof(nova_symbol_t));
  sym->name = strdup(name);
  sym->index = codegen->next_local_index++;
  sym->is_global = false;
  sym->type = NULL;
  sym->next = codegen->locals;
  codegen->locals = sym;
  return sym->index;
}

static void codegen_begin_scope(nova_codegen_t *codegen) {
  if (codegen->scope_depth < 64) {
    codegen->local_index_stack[codegen->scope_depth] =
        codegen->next_local_index;
  }
  codegen->scope_depth++;
}

static void codegen_end_scope(nova_codegen_t *codegen, int line) {
  if (codegen->scope_depth == 0)
    return;

  codegen->scope_depth--;
  int prev_local_index = 0;
  if (codegen->scope_depth < 64) { // Check if we are not at the root scope
    prev_local_index = codegen->local_index_stack[codegen->scope_depth];
  }

  // Pop locals declared in this scope
  int to_pop = codegen->next_local_index - prev_local_index;
  for (int i = 0; i < to_pop; i++) {
    chunk_write_opcode(codegen->chunk, OP_POP, line);
  }

  // Remove symbols at current depth
  nova_symbol_t **pp = &codegen->locals;
  while (*pp) {
    nova_symbol_t *curr = *pp;
    // Remove locals that were added in this scope (their index is >=
    // prev_local_index)
    if (curr->index >= prev_local_index && !curr->is_global) {
      *pp = curr->next;
      free(curr->name);
      free(curr);
    } else {
      pp = &curr->next;
    }
  }

  codegen->next_local_index = prev_local_index;
}

static bool generate_expression(nova_codegen_t *codegen, nova_expr_t *expr) {
  if (!expr)
    return true;

  switch (expr->kind) {
  case EXPR_INT:
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    int constant = chunk_add_constant(codegen->chunk,
                                      value_number((double)expr->data.lit_int));
    chunk_write(codegen->chunk, (uint8_t)constant, expr->span.line);
    break;

  case EXPR_FLOAT:
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    constant =
        chunk_add_constant(codegen->chunk, value_number(expr->data.lit_float));
    chunk_write(codegen->chunk, (uint8_t)constant, expr->span.line);
    break;

  case EXPR_STR:
    // String literal as constant
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    constant =
        chunk_add_constant(codegen->chunk, value_string(expr->data.lit_str));
    chunk_write(codegen->chunk, (uint8_t)constant, expr->span.line);
    break;

  case EXPR_BOOL:
    chunk_write_opcode(codegen->chunk, expr->data.lit_bool ? OP_TRUE : OP_FALSE,
                       expr->span.line);
    break;

  case EXPR_CALL: {
    // Handle enum constructors (e.g. Result::Ok(5))
    if (expr->data.call.func->kind == EXPR_NAMESPACED_ACCESS) {
      int tag, fields;
      if (lookup_variant_tag(
              codegen, expr->data.call.func->data.namespaced_access.member,
              &tag, &fields)) {
        // Generate arguments (push to stack)
        for (size_t i = 0; i < expr->data.call.arg_count; i++) {
          if (!generate_expression(codegen, expr->data.call.args[i]))
            return false;
        }
        chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
        chunk_write(codegen->chunk, (uint8_t)tag, expr->span.line);
        chunk_write(codegen->chunk, (uint8_t)expr->data.call.arg_count,
                    expr->span.line);
        break;
      }
      /* Not an enum variant — treat the member as a regular function name.
       * This handles static method calls like Optimizer::new(). */
    }

    // Generate arguments (push to stack)
    /* If this is a method call (field access as func), push the object first */
    if (expr->data.call.func->kind == EXPR_FIELD_ACCESS) {
      if (!generate_expression(codegen,
                               expr->data.call.func->data.field_access.object))
        return false;
    }

    for (size_t i = 0; i < expr->data.call.arg_count; i++) {
      if (!generate_expression(codegen, expr->data.call.args[i]))
        return false;
    }

    // Lookup function by name
    const char *fn_name = NULL;
    if (expr->data.call.func->kind == EXPR_IDENT) {
      fn_name = expr->data.call.func->data.ident;
    } else if (expr->data.call.func->kind == EXPR_NAMESPACED_ACCESS) {
      fn_name = expr->data.call.func->data.namespaced_access.member;
    } else if (expr->data.call.func->kind == EXPR_FIELD_ACCESS) {
      fn_name = expr->data.call.func->data.field_access.field;
    } else {
      codegen->error_message = strdup("Function call requires identifier");
      return false;
    }

    // Handle builtin functions
    if (strcmp(fn_name, "println") == 0 || strcmp(fn_name, "print") == 0) {
      chunk_write_opcode(codegen->chunk, OP_PRINT, expr->span.line);
      break;
    }

    if (strcmp(fn_name, "Ok") == 0 || strcmp(fn_name, "Some") == 0) {
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, 0, expr->span.line); // Tag 0
      chunk_write(codegen->chunk, (uint8_t)expr->data.call.arg_count,
                  expr->span.line);
      break;
    }

    if (strcmp(fn_name, "Err") == 0) {
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, 1, expr->span.line); // Tag 1
      chunk_write(codegen->chunk, (uint8_t)expr->data.call.arg_count,
                  expr->span.line);
      break;
    }

    /* to_string() as a no-op builtin for bootstrap (just leaves the object) */
    if (strcmp(fn_name, "to_string") == 0) {
      break;
    }

    /* new() as a no-op constructor */
    if (strcmp(fn_name, "new") == 0) {
      // Pop arguments
      for (size_t i = 0; i < expr->data.call.arg_count; i++) {
        chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);
      }
      // Return empty struct
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, 0, expr->span.line); // Tag 0
      chunk_write(codegen->chunk, 0, expr->span.line); // 0 fields
      break;
    }

    if (strcmp(fn_name, "bootstrap_self") == 0 ||
        strcmp(fn_name, "read_to_string") == 0) {
      // Pop arguments
      for (size_t i = 0; i < expr->data.call.arg_count; i++) {
        chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);
      }
      // Return Ok(null)
      chunk_write_opcode(codegen->chunk, OP_NULL, expr->span.line);
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, 0, expr->span.line); // Tag 0 (Ok)
      chunk_write(codegen->chunk, 1, expr->span.line); // 1 field
      break;
    }

    if (strcmp(fn_name, "String_len") == 0 ||
        strcmp(fn_name, "String_from") == 0 ||
        strcmp(fn_name, "String_from_literal") == 0 ||
        strcmp(fn_name, "assert") == 0) {
      /* Treat as a known external — emit a constant 0 placeholder.
       * Semantic/runtime layer resolves these. */
      chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
      int const_idx = chunk_add_constant(codegen->chunk, value_number(0));
      chunk_write(codegen->chunk, (uint8_t)const_idx, expr->span.line);
      break;
    }

    // Regular user-defined function
    int fn_index = -1;
    for (int i = 0; i < codegen->function_count; i++) {
      if (strcmp(codegen->functions[i].name, fn_name) == 0) {
        fn_index = i;
        break;
      }
    }

    if (fn_index == -1) {
      char error[256];
      snprintf(error, sizeof(error), "Undefined function '%s'", fn_name);
      codegen->error_message = strdup(error);
      return false;
    }

    // Generate OP_CALL or OP_VPU_CALL based on target_vpu
    int fn_offset = codegen->functions[fn_index].bytecode_offset;
    int target_vpu = codegen->functions[fn_index].target_vpu;

    int total_args = (int)expr->data.call.arg_count;
    if (expr->data.call.func->kind == EXPR_FIELD_ACCESS) {
      total_args++;
    }

    if (target_vpu != 0) {
      // OP_VPU_CALL <backend_type> <fn_offset_16> <nargs>
      chunk_write_opcode(codegen->chunk, OP_VPU_CALL, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)target_vpu, expr->span.line);
      chunk_write(codegen->chunk, (fn_offset >> 8) & 0xff, expr->span.line);
      chunk_write(codegen->chunk, fn_offset & 0xff, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)total_args, expr->span.line);
    } else {
      // Regular OP_CALL with function offset (not index!)
      chunk_write_opcode(codegen->chunk, OP_CALL, expr->span.line);
      chunk_write(codegen->chunk, (fn_offset >> 8) & 0xff,
                  expr->span.line); // High byte
      chunk_write(codegen->chunk, fn_offset & 0xff,
                  expr->span.line); // Low byte
      chunk_write(codegen->chunk, (uint8_t)total_args, expr->span.line);
    }

    break;
  }

  case EXPR_IDENT: {
    // Lookup variable
    nova_symbol_t *symbol = codegen_lookup_variable(codegen, expr->data.ident);

    if (strcmp(expr->data.ident, "None") == 0) {
      chunk_write_opcode(codegen->chunk, OP_NULL, expr->span.line);
      break;
    }

    if (symbol && symbol->is_global) {
      chunk_write_opcode(codegen->chunk, OP_GET_GLOBAL, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)symbol->index, expr->span.line);
    } else if (symbol) {
      chunk_write_opcode(codegen->chunk, OP_GET_LOCAL, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)symbol->index, expr->span.line);
    } else {
      // Not found, push 0
      chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
      int constant = chunk_add_constant(codegen->chunk, value_number(0));
      chunk_write(codegen->chunk, (uint8_t)constant, expr->span.line);
    }
    break;
  }

  case EXPR_ASSIGN: {
    // Generate the value
    if (!generate_expression(codegen, expr->data.assign.value))
      return false;

    // Lookup variable
    nova_symbol_t *symbol =
        codegen_lookup_variable(codegen, expr->data.assign.name);

    // Generate SET opcode based on scope
    if (symbol && symbol->is_global) {
      chunk_write_opcode(codegen->chunk, OP_SET_GLOBAL, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)symbol->index, expr->span.line);
    } else if (symbol) {
      chunk_write_opcode(codegen->chunk, OP_SET_LOCAL, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)symbol->index, expr->span.line);
    } else {
      chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);
    }
    break;
  }

  case EXPR_ADDR_OF: {
    // &x - get address of variable
    // For now, we'll use variable index as "address"
    // In a real implementation, this would need heap/stack addresses
    if (expr->data.addr_of->kind != EXPR_IDENT) {
      codegen->error_message =
          strdup("Can only take address of variables (for now)");
      return false;
    }

    nova_symbol_t *symbol =
        codegen_lookup_variable(codegen, expr->data.addr_of->data.ident);
    if (!symbol) {
      char error[256];
      snprintf(error, sizeof(error), "Undefined variable '%s'",
               expr->data.addr_of->data.ident);
      codegen->error_message = strdup(error);
      return false;
    }

    chunk_write_opcode(codegen->chunk, OP_ADDR_OF, expr->span.line);
    chunk_write(codegen->chunk, (uint8_t)symbol->index, expr->span.line);
    break;
  }

  case EXPR_DEREF: {
    // *p - dereference pointer
    // Generate the pointer expression
    if (!generate_expression(codegen, expr->data.deref))
      return false;

    // Emit dereference opcode
    chunk_write_opcode(codegen->chunk, OP_DEREF, expr->span.line);
    break;
  }

  case EXPR_BINARY: {
    const char *op = expr->data.binary.op;

    // Special case for assignment via binary expression (e.g. *p = v, a[i] = v)
    if (strcmp(op, "=") == 0) {
      nova_expr_t *left = expr->data.binary.left;
      nova_expr_t *right = expr->data.binary.right;

      if (left->kind == EXPR_INDEX) {
        // array[index] = value
        if (!generate_expression(codegen, left->data.index.object))
          return false;
        if (!generate_expression(codegen, left->data.index.index))
          return false;
        if (!generate_expression(codegen, right))
          return false;

        chunk_write_opcode(codegen->chunk, OP_INDEX_SET, expr->span.line);
        return true;
      } else if (left->kind == EXPR_DEREF) {
        // *ptr = value
        if (!generate_expression(codegen, left->data.deref))
          return false;
        if (!generate_expression(codegen, right))
          return false;

        chunk_write_opcode(codegen->chunk, OP_STORE_PTR, expr->span.line);
        return true;
      } else {
        codegen->error_message = strdup("Unsupported assignment target");
        return false;
      }
    }

    /* ── Compile-time constant folding ──────────────────────────────────
     * If both operands are compile-time constants fold into single OP_CONSTANT.
     * Examples: 5.0.km * 2.0 → OP_CONSTANT(10000.0)
     *           3.0.kg + 2.0.kg → OP_CONSTANT(5.0) [SI values added]
     */
    double folded;
    if (try_fold_binary(expr, &folded)) {
      chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
      int fold_ci = chunk_add_constant(codegen->chunk, value_number(folded));
      chunk_write(codegen->chunk, (uint8_t)fold_ci, expr->span.line);
      return true;
    }

    // Generate operands
    if (!generate_expression(codegen, expr->data.binary.left))
      return false;
    if (!generate_expression(codegen, expr->data.binary.right))
      return false;

    // Generate operation
    if (strcmp(op, "+") == 0) {
      chunk_write_opcode(codegen->chunk, OP_ADD, expr->span.line);
    } else if (strcmp(op, "-") == 0) {
      chunk_write_opcode(codegen->chunk, OP_SUBTRACT, expr->span.line);
    } else if (strcmp(op, "*") == 0) {
      chunk_write_opcode(codegen->chunk, OP_MULTIPLY, expr->span.line);
    } else if (strcmp(op, "/") == 0) {
      chunk_write_opcode(codegen->chunk, OP_DIVIDE, expr->span.line);
    } else if (strcmp(op, "%") == 0) {
      chunk_write_opcode(codegen->chunk, OP_MODULO, expr->span.line);
    } else if (strcmp(op, ">") == 0) {
      chunk_write_opcode(codegen->chunk, OP_GREATER, expr->span.line);
    } else if (strcmp(op, "<") == 0) {
      chunk_write_opcode(codegen->chunk, OP_LESS, expr->span.line);
    } else if (strcmp(op, ">=") == 0) {
      chunk_write_opcode(codegen->chunk, OP_GREATER_EQUAL, expr->span.line);
    } else if (strcmp(op, "<=") == 0) {
      chunk_write_opcode(codegen->chunk, OP_LESS_EQUAL, expr->span.line);
    } else if (strcmp(op, "==") == 0) {
      chunk_write_opcode(codegen->chunk, OP_EQUAL, expr->span.line);
    } else if (strcmp(op, "!=") == 0) {
      chunk_write_opcode(codegen->chunk, OP_NOT_EQUAL, expr->span.line);
    } else if (strcmp(op, "in") == 0) {
      /*
       * Unit conversion: `expr in target_unit`
       * e.g. `10.0.km in m`  →  10000.0
       *      `5.0.kg in g`   →  5000.0
       *
       * Strategy (compile-time):
       * The left side already has an SI-normalised value on the stack (from
       * EXPR_UNIT_LITERAL or a qty variable). The right side is the target unit
       * identifier (EXPR_IDENT with unit name like "m", "g", etc.).
       *
       * We compute the to-scale at compile time and emit
       * OP_UNIT_SCALE(1/to_scale) to de-normalise into the target unit.  If
       * to_scale == 1.0 we skip it.
       */
      nova_expr_t *rhs = expr->data.binary.right;
      const char *target_unit = NULL;
      if (rhs->kind == EXPR_IDENT) {
        target_unit = rhs->data.ident;
      }

      if (target_unit) {
        nova_dimension_t *to_dim = nova_dim_parse(target_unit);
        if (to_dim) {
          double to_scale = nova_dim_get_scale(to_dim);
          nova_dim_destroy(to_dim);
          /* Stack already has SI value from left side (EXPR_UNIT_LITERAL
           * handled above). We need to pop the right-side IDENT value that was
           * emitted above — but we skipped generate_expression for RHS here.
           * The generates above already ran both sides, so we need to undo the
           * RHS push. */
          /* Pop the spurious RHS constant (IDENT was pushed as 0) */
          chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);
          /* Now apply inverse scale: SI_value / to_scale = target_unit_value */
          if (to_scale != 1.0 && !isnan(to_scale) && to_scale != 0.0) {
            chunk_write_opcode(codegen->chunk, OP_UNIT_SCALE, expr->span.line);
            int inv_const = chunk_add_constant(codegen->chunk,
                                               value_number(1.0 / to_scale));
            chunk_write(codegen->chunk, (uint8_t)inv_const, expr->span.line);
          }
        } else {
          /* Unknown target unit — just divide (keep default OP_DIVIDE fallback)
           */
          chunk_write_opcode(codegen->chunk, OP_DIVIDE, expr->span.line);
        }
      } else {
        /* RHS is not an ident — runtime divide fallback */
        chunk_write_opcode(codegen->chunk, OP_DIVIDE, expr->span.line);
      }
    } else {
      codegen->error_message = strdup("Unknown binary operator");
      return false;
    }
    break;
  }

  case EXPR_ARRAY_LIT: {
    for (size_t i = 0; i < expr->data.array_lit.count; i++) {
      if (!generate_expression(codegen, expr->data.array_lit.elements[i]))
        return false;
    }
    chunk_write_opcode(codegen->chunk, OP_ARRAY_LIT, expr->span.line);
    chunk_write(codegen->chunk, (uint8_t)expr->data.array_lit.count,
                expr->span.line);
    break;
  }

  case EXPR_INDEX: {
    if (!generate_expression(codegen, expr->data.index.object))
      return false;
    if (!generate_expression(codegen, expr->data.index.index))
      return false;
    chunk_write_opcode(codegen->chunk, OP_INDEX_GET, expr->span.line);
    break;
  }

  case EXPR_STRING_LEN: {
    if (!generate_expression(codegen, expr->data.string_len))
      return false;
    chunk_write_opcode(codegen->chunk, OP_ARRAY_LEN, expr->span.line);
    break;
  }

  case EXPR_NAMESPACED_ACCESS: {
    // Handle enum variants with 0 fields (like None)
    int tag, fields;
    if (lookup_variant_tag(codegen, expr->data.namespaced_access.member, &tag,
                           &fields)) {
      if (fields == 0) {
        chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
        chunk_write(codegen->chunk, (uint8_t)tag, expr->span.line);
        chunk_write(codegen->chunk, 0, expr->span.line); // 0 fields
        break;
      }
    }

    // Fallback: treat as a constant hash for unknown namespaced
    // constants/variants
    int hash = 0;
    const char *name = expr->data.namespaced_access.member;
    for (const char *p = name; *p; p++) {
      hash = (hash * 31 + *p) & 0xFF;
    }
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    int const_idx =
        chunk_add_constant(codegen->chunk, value_number((double)hash));
    chunk_write(codegen->chunk, (uint8_t)const_idx, expr->span.line);
    break;
  }

  case EXPR_STRUCT_INIT: {
    // 1. Find struct definition to get field order
    struct_entry_t *s = NULL;
    for (int i = 0; i < codegen->struct_count; i++) {
      if (strcmp(codegen->structs[i].name,
                 expr->data.struct_init.struct_name) == 0) {
        s = &codegen->structs[i];
        break;
      }
    }

    if (!s) {
      // Fallback: use fields in order they appear in init
      for (size_t i = 0; i < expr->data.struct_init.field_count; i++) {
        if (!generate_expression(codegen,
                                 expr->data.struct_init.fields[i].value))
          return false;
      }
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, 0, expr->span.line); // Tag 0 for structs
      chunk_write(codegen->chunk, (uint8_t)expr->data.struct_init.field_count,
                  expr->span.line);
    } else {
      // Push fields in declaration order
      for (int i = 0; i < s->field_count; i++) {
        bool found = false;
        for (size_t j = 0; j < expr->data.struct_init.field_count; j++) {
          if (strcmp(expr->data.struct_init.fields[j].name,
                     s->field_names[i]) == 0) {
            if (!generate_expression(codegen,
                                     expr->data.struct_init.fields[j].value))
              return false;
            found = true;
            break;
          }
        }
        if (!found) {
          // Push null if field missing
          chunk_write_opcode(codegen->chunk, OP_NULL, expr->span.line);
        }
      }
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, 0, expr->span.line); // Tag 0 for structs
      chunk_write(codegen->chunk, (uint8_t)s->field_count, expr->span.line);
    }
    break;
  }

  case EXPR_FIELD_ACCESS: {
    if (!generate_expression(codegen, expr->data.field_access.object))
      return false;

    // TODO: Use actual field index from struct definition
    int field_idx = -1;
    // We don't have type info here easily, so we might need a better way.
    // For now, if we can find any struct with this field name, use it?
    // This is a hack for bootstrap.
    for (int i = 0; i < codegen->struct_count; i++) {
      for (int j = 0; j < codegen->structs[i].field_count; j++) {
        if (strcmp(codegen->structs[i].field_names[j],
                   expr->data.field_access.field) == 0) {
          field_idx = j;
          break;
        }
      }
      if (field_idx != -1)
        break;
    }

    if (field_idx == -1) {
      // Simple heuristic if not found: maybe it's 0?
      field_idx = 0;
    }

    chunk_write_opcode(codegen->chunk, OP_GET_FIELD, expr->span.line);
    chunk_write(codegen->chunk, (uint8_t)field_idx, expr->span.line);
    break;
  }

  case EXPR_MATCH: {
    // Evaluate target
    if (!generate_expression(codegen, expr->data.match.target))
      return false;

    int *exit_jumps = malloc(sizeof(int) * expr->data.match.arm_count);
    int exit_jump_count = 0;

    for (size_t i = 0; i < expr->data.match.arm_count; i++) {
      nova_match_arm_t *arm = expr->data.match.arms[i];

      // DUP target for check
      chunk_write_opcode(codegen->chunk, OP_DUP, arm->pattern->span.line);

      if (!generate_pattern_check(codegen, arm->pattern)) {
        free(exit_jumps);
        return false;
      }

      // Pattern check left a bool. Jump if false to next arm.
      chunk_write_opcode(codegen->chunk, OP_JUMP_IF_FALSE,
                         arm->pattern->span.line);
      int false_jump = codegen->chunk->count;
      chunk_write(codegen->chunk, 0xff, arm->pattern->span.line);
      chunk_write(codegen->chunk, 0xff, arm->pattern->span.line);

      // Match! Pop original target (it's below the body result)
      // Wait, we should pop it BEFORE executing body if we don't need it.
      // But body might need it for bindings (not implemented yet).
      // For now, pop it.
      chunk_write_opcode(codegen->chunk, OP_POP, arm->body->span.line);

      if (!generate_expression(codegen, arm->body)) {
        free(exit_jumps);
        return false;
      }

      // Jump to end of match
      chunk_write_opcode(codegen->chunk, OP_JUMP, arm->body->span.line);
      exit_jumps[exit_jump_count++] = codegen->chunk->count;
      chunk_write(codegen->chunk, 0xff, arm->body->span.line);
      chunk_write(codegen->chunk, 0xff, arm->body->span.line);

      // False path starts here
      int false_target = codegen->chunk->count;
      codegen->chunk->code[false_jump] =
          ((false_target - false_jump - 2) >> 8) & 0xff;
      codegen->chunk->code[false_jump + 1] =
          (false_target - false_jump - 2) & 0xff;
    }

    // No match! Pop original target and push NULL
    chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);
    chunk_write_opcode(codegen->chunk, OP_NULL, expr->span.line);

    // Patch exit jumps
    int exit_target = codegen->chunk->count;
    for (int i = 0; i < exit_jump_count; i++) {
      int jump_off = exit_jumps[i];
      codegen->chunk->code[jump_off] =
          ((exit_target - jump_off - 2) >> 8) & 0xff;
      codegen->chunk->code[jump_off + 1] = (exit_target - jump_off - 2) & 0xff;
    }

    free(exit_jumps);
    break;
  }

  case EXPR_UNIT_LITERAL: {
    /*
     * Unit literal: e.g. 5.0.kg, 9.81.m/s2
     * Strategy: emit the numeric value as a constant, then if scale_factor
     * != 1.0, emit OP_UNIT_SCALE to convert to SI base units at load time.
     *
     * Examples:
     *   5.0.kg  → value=5.0, unit="kg"  → scale=1.0 → just push 5.0
     *   10.0.km → value=10.0, unit="km" → scale=1000.0 → push 10.0,
     * OP_UNIT_SCALE(1000) 9.81.m  → value=9.81, unit="m"  → scale=1.0 → just
     * push 9.81
     */
    double raw_value = expr->data.unit_literal.value;
    const char *unit = expr->data.unit_literal.unit;

    /* Push raw value */
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    int val_const = chunk_add_constant(codegen->chunk, value_number(raw_value));
    chunk_write(codegen->chunk, (uint8_t)val_const, expr->span.line);

    /* Apply scale factor if unit has a non-unity scale (e.g. km, cm, g) */
    if (unit && *unit) {
      nova_dimension_t *dim = nova_dim_parse(unit);
      if (dim) {
        double scale = nova_dim_get_scale(dim);
        nova_dim_destroy(dim);
        if (scale != 1.0 && !isnan(scale) && scale != 0.0) {
          /* Emit OP_UNIT_SCALE with scale constant */
          chunk_write_opcode(codegen->chunk, OP_UNIT_SCALE, expr->span.line);
          int scale_const =
              chunk_add_constant(codegen->chunk, value_number(scale));
          chunk_write(codegen->chunk, (uint8_t)scale_const, expr->span.line);
        }
      }
    }
    break;
  }

  case EXPR_ENUM_VARIANT: {
    const char *name = expr->data.enum_variant.variant_name;
    int tag, fields;

    if (name && lookup_variant_tag(codegen, name, &tag, &fields)) {
      // Found a registered variant - use proper OP_ENUM
      // 1. Push arguments
      for (size_t i = 0; i < expr->data.enum_variant.arg_count; i++) {
        if (!generate_expression(codegen, expr->data.enum_variant.args[i]))
          return false;
      }
      // 2. Emit OP_ENUM <tag> <count>
      chunk_write_opcode(codegen->chunk, OP_ENUM, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)tag, expr->span.line);
      chunk_write(codegen->chunk, (uint8_t)expr->data.enum_variant.arg_count,
                  expr->span.line);
    } else {
      // Fallback to simple hash (if variant not registered yet)
      int variant_id = 0;
      if (name) {
        for (const char *p = name; *p; p++) {
          variant_id = (variant_id * 31 + *p) & 0xFF;
        }
      }

      chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
      int constant_idx =
          chunk_add_constant(codegen->chunk, value_number(variant_id));
      chunk_write(codegen->chunk, (uint8_t)constant_idx, expr->span.line);

      if (expr->data.enum_variant.arg_count > 0) {
        for (size_t i = 0; i < expr->data.enum_variant.arg_count; i++) {
          if (!generate_expression(codegen, expr->data.enum_variant.args[i]))
            return false;
        }
      }
    }
    break;
  }

  default: {
    char error[256];
    snprintf(error, sizeof(error), "Unsupported expression kind %d",
             expr->kind);
    codegen->error_message = strdup(error);
    return false;
  }
  }
  return true;
}

static bool generate_statement(nova_codegen_t *codegen, nova_stmt_t *stmt) {
  if (!stmt)
    return true;

  switch (stmt->kind) {
  case STMT_EXPR:
    if (!generate_expression(codegen, stmt->data.expr))
      return false;
    // Pop the result of the expression
    chunk_write_opcode(codegen->chunk, OP_POP, stmt->span.line);
    break;

  case STMT_CHECK: {
    // Generate condition
    if (!generate_expression(codegen, stmt->data.check_stmt.condition))
      return false;

    // Jump if false - we'll patch this later
    chunk_write_opcode(codegen->chunk, OP_JUMP_IF_FALSE, stmt->span.line);
    int then_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, stmt->span.line); // Placeholder
    chunk_write(codegen->chunk, 0xff, stmt->span.line); // 16-bit offset

    // Generate then branch
    if (!generate_statement(codegen, stmt->data.check_stmt.then_branch))
      return false;

    // Jump over else branch (if condition was true)
    chunk_write_opcode(codegen->chunk, OP_JUMP, stmt->span.line);
    int else_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, stmt->span.line); // Placeholder
    chunk_write(codegen->chunk, 0xff, stmt->span.line);

    // Patch then jump to here (false path starts here)
    int then_target = codegen->chunk->count;
    codegen->chunk->code[then_jump] =
        ((then_target - then_jump - 2) >> 8) & 0xff;
    codegen->chunk->code[then_jump + 1] = (then_target - then_jump - 2) & 0xff;

    // Generate optional else branch
    if (stmt->data.check_stmt.else_branch) {
      if (!generate_statement(codegen, stmt->data.check_stmt.else_branch))
        return false;
    }

    // Patch else jump to end
    int else_target = codegen->chunk->count;
    codegen->chunk->code[else_jump] =
        ((else_target - else_jump - 2) >> 8) & 0xff;
    codegen->chunk->code[else_jump + 1] = (else_target - else_jump - 2) & 0xff;

    break;
  }

  case STMT_FN: {
    // First emit jump to skip over function body
    chunk_write_opcode(codegen->chunk, OP_JUMP, stmt->span.line);
    int skip_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, stmt->span.line); // Placeholder
    chunk_write(codegen->chunk, 0xff, stmt->span.line);

    // Store function in table
    if (codegen->function_count >= codegen->function_capacity) {
      codegen->function_capacity =
          codegen->function_capacity == 0 ? 8 : codegen->function_capacity * 2;
      codegen->functions =
          realloc(codegen->functions,
                  sizeof(function_entry_t) * codegen->function_capacity);
    }

    function_entry_t *fn = &codegen->functions[codegen->function_count++];
    fn->name = strdup(stmt->data.fn_stmt.name);
    fn->param_count = (int)stmt->data.fn_stmt.param_count;
    fn->bytecode_offset = codegen->chunk->count; // Function starts here
    fn->target_vpu = stmt->data.fn_stmt.target_vpu;

    // fprintf(stderr, "[CODEGEN] Function '%s' at offset %d with %d params\n",
    // fn->name,
    //         fn->bytecode_offset, fn->param_count);

    // Register parameters as locals
    codegen_begin_scope(codegen);
    for (size_t i = 0; i < stmt->data.fn_stmt.param_count; i++) {
      codegen_add_local(codegen, stmt->data.fn_stmt.params[i]->name);
    }

    // Pop arguments from stack into parameter locals
    // Arguments are on stack in reverse order (last arg on top)
    // So we need to pop them in reverse and store to locals
    for (int i = (int)stmt->data.fn_stmt.param_count - 1; i >= 0; i--) {
      chunk_write_opcode(codegen->chunk, OP_SET_LOCAL, stmt->span.line);
      chunk_write(codegen->chunk, (uint8_t)i, stmt->span.line);
      chunk_write_opcode(codegen->chunk, OP_POP, stmt->span.line);
    }

    if (stmt->data.fn_stmt.body) {
      if (!generate_statement(codegen, stmt->data.fn_stmt.body)) {
        codegen_end_scope(codegen, stmt->span.line); // Clean up locals on error
        return false;
      }
    }

    // Add implicit null and return if not present
    chunk_write_opcode(codegen->chunk, OP_NULL, stmt->span.line);
    chunk_write_opcode(codegen->chunk, OP_RETURN, stmt->span.line);

    codegen_end_scope(codegen, stmt->span.line);

    // Patch the skip jump to point here (after function)
    int skip_target = codegen->chunk->count;
    int jump_distance = skip_target - skip_jump - 2;
    codegen->chunk->code[skip_jump] = (jump_distance >> 8) & 0xff;
    codegen->chunk->code[skip_jump + 1] = jump_distance & 0xff;

    break;
  }

  case STMT_RETURN: {
    /* C-style `return expr;` — identical codegen to yield */
    nova_stmt_t yield_alias = *stmt;
    yield_alias.kind = STMT_YIELD;
    yield_alias.data.yield_stmt = stmt->data.return_expr;
    return generate_statement(codegen, &yield_alias);
  }

  case STMT_YIELD: {
    // Generate return value (if any)
    if (stmt->data.yield_stmt) {
      if (!generate_expression(codegen, stmt->data.yield_stmt))
        return false;
    } else {
      // Return null if no value
      chunk_write_opcode(codegen->chunk, OP_NULL, stmt->span.line);
    }

    // Generate return instruction
    chunk_write_opcode(codegen->chunk, OP_RETURN, stmt->span.line);
    break;
  }

  case STMT_BREAK: {
    if (!codegen->current_loop) {
      codegen->error_message = strdup("'break' outside of loop");
      return false;
    }

    // Emit jump placeholder - will be patched at end of loop
    chunk_write_opcode(codegen->chunk, OP_JUMP, stmt->span.line);
    int jump_offset = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, stmt->span.line);
    chunk_write(codegen->chunk, 0xff, stmt->span.line);

    // Record this break jump to patch later
    loop_context_add_break(codegen->current_loop, jump_offset);
    break;
  }

  case STMT_CONTINUE: {
    if (!codegen->current_loop) {
      codegen->error_message = strdup("'continue' outside of loop");
      return false;
    }

    // Emit OP_LOOP to jump back to loop start
    chunk_write_opcode(codegen->chunk, OP_LOOP, stmt->span.line);
    int loop_offset =
        (codegen->chunk->count + 2) - codegen->current_loop->loop_start;
    chunk_write(codegen->chunk, (loop_offset >> 8) & 0xff, stmt->span.line);
    chunk_write(codegen->chunk, loop_offset & 0xff, stmt->span.line);
    break;
  }

  case STMT_VAR_DECL: {
    // Generate initializer
    if (stmt->data.var_decl.init) {
      if (!generate_expression(codegen, stmt->data.var_decl.init))
        return false;
    } else {
      // Default to null
      chunk_write_opcode(codegen->chunk, OP_NULL, stmt->span.line);
    }

    // Lookup variable to get its index
    nova_symbol_t *symbol = nova_semantic_lookup_variable(
        codegen->semantic, stmt->data.var_decl.name);
    bool is_global = (symbol != NULL && symbol->is_global);

    if (is_global) {
      chunk_write_opcode(codegen->chunk, OP_DEFINE_GLOBAL, stmt->span.line);
      chunk_write(codegen->chunk, (uint8_t)symbol->index, stmt->span.line);
    } else {
      // Local variable: initializer already pushed value to stack.
      // Add to our local table.
      codegen_add_local(codegen, stmt->data.var_decl.name);
    }
    break;
  }

  case STMT_ENUM_DECL:
    codegen_register_enum(codegen, stmt);
    break;

  case STMT_STRUCT_DECL:
    codegen_register_struct(codegen, stmt);
    break;

  case STMT_BLOCK: {
    codegen_begin_scope(codegen);
    // Generate all statements in the block
    for (size_t i = 0; i < stmt->data.block.count; i++) {
      if (!generate_statement(codegen, stmt->data.block.statements[i])) {
        return false;
      }
    }
    codegen_end_scope(codegen, stmt->span.line);
    break;
  }

  case STMT_WHILE: {
    // Loop başlangıç noktası
    int loop_start = codegen->chunk->count;

    // Create loop context (for break/continue)
    loop_context_t *loop_ctx =
        loop_context_create(loop_start, codegen->current_loop);
    loop_context_t *prev_loop = codegen->current_loop;
    codegen->current_loop = loop_ctx;

    // Condition üret
    if (!generate_expression(codegen, stmt->data.while_stmt.condition)) {
      codegen->current_loop = prev_loop;
      loop_context_destroy(loop_ctx);
      return false;
    }

    // Eğer false ise loop'tan çık
    chunk_write_opcode(codegen->chunk, OP_JUMP_IF_FALSE, stmt->span.line);
    int exit_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, stmt->span.line);
    chunk_write(codegen->chunk, 0xff, stmt->span.line);

    // Body üret
    if (!generate_statement(codegen, stmt->data.while_stmt.body)) {
      codegen->current_loop = prev_loop;
      loop_context_destroy(loop_ctx);
      return false;
    }

    // Geri zıpla (OP_LOOP backward jump)
    chunk_write_opcode(codegen->chunk, OP_LOOP, stmt->span.line);

    // Offset = how far back to jump from position after reading offset bytes
    int loop_offset = (codegen->chunk->count + 2) - loop_start;
    chunk_write(codegen->chunk, (loop_offset >> 8) & 0xff, stmt->span.line);
    chunk_write(codegen->chunk, loop_offset & 0xff, stmt->span.line);

    // Patch exit jump
    int exit_target = codegen->chunk->count;
    codegen->chunk->code[exit_jump] =
        ((exit_target - exit_jump - 2) >> 8) & 0xff;
    codegen->chunk->code[exit_jump + 1] = (exit_target - exit_jump - 2) & 0xff;

    // Patch breaks
    for (int i = 0; i < loop_ctx->break_count; i++) {
      int offset = loop_ctx->break_jumps[i]; // Corrected from break_offsets
      codegen->chunk->code[offset] = ((exit_target - offset - 2) >> 8) & 0xff;
      codegen->chunk->code[offset + 1] = (exit_target - offset - 2) & 0xff;
    }

    // Patch all continue jumps to loop start
    for (int i = 0; i < loop_ctx->continue_count; i++) {
      int continue_offset = loop_ctx->continue_jumps[i];
      int continue_distance =
          (codegen->chunk->count + 2) - loop_start; // Jump to condition check
      codegen->chunk->code[continue_offset] = (continue_distance >> 8) & 0xff;
      codegen->chunk->code[continue_offset + 1] = continue_distance & 0xff;
    }

    // Restore previous loop context
    codegen->current_loop = prev_loop;
    loop_context_destroy(loop_ctx);

    break;
  }

  case STMT_FOR: {
    codegen_begin_scope(codegen);

    // 1. Initialize binding: var binding = start;
    if (!generate_expression(codegen, stmt->data.for_stmt.start))
      return false;
    int iter_index = codegen_add_local(codegen, stmt->data.for_stmt.binding);

    // 2. Loop start
    int loop_start = codegen->chunk->count;

    // Context
    loop_context_t *loop_ctx =
        loop_context_create(loop_start, codegen->current_loop);
    loop_context_t *prev_loop = codegen->current_loop;
    codegen->current_loop = loop_ctx;

    // 3. Condition: binding < end
    chunk_write_opcode(codegen->chunk, OP_GET_LOCAL, stmt->span.line);
    chunk_write(codegen->chunk, (uint8_t)iter_index, stmt->span.line);
    if (!generate_expression(codegen, stmt->data.for_stmt.end)) {
      codegen->current_loop = prev_loop;
      loop_context_destroy(loop_ctx);
      return false;
    }
    chunk_write_opcode(codegen->chunk, OP_LESS, stmt->span.line);

    // 4. Jump if false
    chunk_write_opcode(codegen->chunk, OP_JUMP_IF_FALSE, stmt->span.line);
    int exit_jump = codegen->chunk->count;
    chunk_write(codegen->chunk, 0xff, stmt->span.line);
    chunk_write(codegen->chunk, 0xff, stmt->span.line);

    // 5. Body
    if (!generate_statement(codegen, stmt->data.for_stmt.body)) {
      codegen->current_loop = prev_loop;
      loop_context_destroy(loop_ctx);
      return false;
    }

    // 6. Increment: i = i + 1
    chunk_write_opcode(codegen->chunk, OP_GET_LOCAL, stmt->span.line);
    chunk_write(codegen->chunk, (uint8_t)iter_index, stmt->span.line);
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, stmt->span.line);
    int one_const = chunk_add_constant(codegen->chunk, value_number(1.0));
    chunk_write(codegen->chunk, (uint8_t)one_const, stmt->span.line);
    chunk_write_opcode(codegen->chunk, OP_ADD, stmt->span.line);
    chunk_write_opcode(codegen->chunk, OP_SET_LOCAL, stmt->span.line);
    chunk_write(codegen->chunk, (uint8_t)iter_index, stmt->span.line);
    chunk_write_opcode(codegen->chunk, OP_POP,
                       stmt->span.line); // Pop assignment result

    // 7. Loop back
    chunk_write_opcode(codegen->chunk, OP_LOOP, stmt->span.line);
    int loop_offset = (codegen->chunk->count + 2) - loop_start;
    chunk_write(codegen->chunk, (loop_offset >> 8) & 0xff, stmt->span.line);
    chunk_write(codegen->chunk, loop_offset & 0xff, stmt->span.line);

    // 8. Patch exit jump
    int exit_target = codegen->chunk->count;
    int jump_distance = exit_target - exit_jump - 2;
    codegen->chunk->code[exit_jump] = (jump_distance >> 8) & 0xff;
    codegen->chunk->code[exit_jump + 1] = jump_distance & 0xff;

    // 9. Patch break jumps
    for (int i = 0; i < loop_ctx->break_count; i++) {
      int break_offset = loop_ctx->break_jumps[i];
      int break_distance = exit_target - break_offset - 2;
      codegen->chunk->code[break_offset] = (break_distance >> 8) & 0xff;
      codegen->chunk->code[break_offset + 1] = break_distance & 0xff;
    }

    // 10. Patch continue jumps (jump back to condition check)
    for (int i = 0; i < loop_ctx->continue_count; i++) {
      int continue_offset = loop_ctx->continue_jumps[i];
      int continue_distance = (codegen->chunk->count + 2) - loop_start;
      codegen->chunk->code[continue_offset] = (continue_distance >> 8) & 0xff;
      codegen->chunk->code[continue_offset + 1] = continue_distance & 0xff;
    }

    // Restore loop context
    codegen->current_loop = prev_loop;
    loop_context_destroy(loop_ctx);
    codegen_end_scope(codegen, stmt->span.line);
    break;
  }

  default:
    // For unhandled statement types, skip
    break;
  }

  return true;
}

static bool generate_program(nova_codegen_t *codegen, nova_program_t *program) {
  // Generate all declarations (statements wrapped as top-level declarations)
  // fprintf(stderr, "[CODEGEN] Generating %zu declarations\n",
  // program->declaration_count);

  // Generate all declarations normally - VM will execute from start
  for (size_t i = 0; i < program->declaration_count; i++) {

    nova_top_level_t *decl = program->declarations[i];
    // fprintf(stderr, "[CODEGEN] Declaration %zu: fn_decl=%p\n", i,
    //         (void *) ((nova_stmt_t *) decl->data));
    if (((nova_stmt_t *)decl->data)) {
      // fprintf(stderr, "[CODEGEN]   Statement kind: %d\n", ((nova_stmt_t *)
      // decl->data)->kind); Generate any statement type (STMT_VAR, STMT_EXPR,
      // etc.)
      if (!generate_statement(codegen, ((nova_stmt_t *)decl->data))) {
        // fprintf(stderr, "[CODEGEN]   ERROR generating statement!\n");
        return false;
      }
      // fprintf(stderr, "[CODEGEN]   Statement generated successfully\n");
    }
  }

  // All functions generated, now call main
  int main_index = -1;
  for (int i = 0; i < codegen->function_count; i++) {
    if (strcmp(codegen->functions[i].name, "main") == 0) {
      main_index = i;
      break;
    }
  }

  if (main_index != -1) {
    // Call main with 0 arguments
    int main_offset = codegen->functions[main_index].bytecode_offset;
    chunk_write_opcode(codegen->chunk, OP_CALL, 0);
    chunk_write(codegen->chunk, (main_offset >> 8) & 0xff, 0);
    chunk_write(codegen->chunk, main_offset & 0xff, 0);
    chunk_write(codegen->chunk, 0, 0); // 0 arguments
  }

  // Add return at the end
  chunk_write_opcode(codegen->chunk, OP_RETURN, 0);

  return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ══════════════════════════════════════════════════════════════════════════════

// Forward declaration for legacy AST-based codegen
bool nova_codegen_generate_from_ast(nova_codegen_t *codegen,
                                    nova_program_t *ast);

nova_codegen_t *nova_codegen_create(nova_backend_type_t backend) {
  // Log the selected backend
  switch (backend) {
  case NOVA_BACKEND_GPU_ARMY_4LUA:
    printf("🦅 [CODEGEN] Backend selected: GPU-Army 4LUA (Tiered Compute)\n");
    break;
  case NOVA_BACKEND_MLIR_CODEGEN:
    printf("🔗 [CODEGEN] Backend selected: MLIR Codegen\n");
    break;
  case NOVA_BACKEND_VM:
    printf("📦 [CODEGEN] Backend selected: VM (Bytecode)\n");
    break;
  case NOVA_BACKEND_JIT:
    printf("⚡ [CODEGEN] Backend selected: JIT (Native)\n");
    break;
  default:
    printf("🔧 [CODEGEN] Backend selected: %d\n", backend);
    break;
  }

  nova_codegen_t *codegen = (nova_codegen_t *)malloc(sizeof(nova_codegen_t));
  if (!codegen)
    return NULL;

  codegen->chunk = (Chunk *)malloc(sizeof(Chunk));
  if (!codegen->chunk) {
    free(codegen);
    return NULL;
  }

  chunk_init(codegen->chunk);
  codegen->error_message = NULL;
  codegen->semantic = NULL;

  // Initialize function table
  codegen->functions = NULL;
  codegen->function_count = 0;
  codegen->function_capacity = 0;

  // Initialize loop context
  codegen->current_loop = NULL;

  // Initialize local variable tracking
  codegen->locals = NULL;
  codegen->next_local_index = 0;
  codegen->scope_depth = 1; // Start at scope depth 1 (global)

  // Initialize enum table
  codegen->enums = NULL;
  codegen->enum_count = 0;
  codegen->enum_capacity = 0;

  return codegen;
}

int nova_codegen_generate(nova_codegen_t *codegen, nova_ir_module_t *ir) {
  (void)ir; // Unused for now - we'll use AST directly in main.c
  // This is a placeholder - actual implementation will come later
  return 0; // Success
}

// Legacy function for AST-based codegen (used by main.c)
bool nova_codegen_generate_from_ast(nova_codegen_t *codegen,
                                    nova_program_t *ast) {
  return generate_program(codegen, ast);
}

// Set semantic analyzer for symbol table access
void nova_codegen_set_semantic(nova_codegen_t *codegen,
                               nova_semantic_t *semantic) {
  codegen->semantic = semantic;
}

// Legacy helper functions for main.c
Chunk *nova_codegen_get_chunk(nova_codegen_t *codegen) {
  return codegen->chunk;
}

const char *nova_codegen_get_error(nova_codegen_t *codegen) {
  return codegen->error_message ? codegen->error_message : "No error";
}

void nova_codegen_destroy(nova_codegen_t *codegen) {
  if (codegen->functions) {
    for (int i = 0; i < codegen->function_count; i++) {
      free(codegen->functions[i].name);
    }
    free(codegen->functions);
  }
  if (codegen->enums) {
    for (int i = 0; i < codegen->enum_count; i++) {
      free(codegen->enums[i].name);
      for (int j = 0; j < codegen->enums[i].variant_count; j++) {
        free(codegen->enums[i].variants[j].name);
      }
      free(codegen->enums[i].variants);
    }
    free(codegen->enums);
  }
  free(codegen);
}
