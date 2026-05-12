/**
 * @file ast.c
 * @brief AST implementation for Nova language
 */

#include "compiler/ast.h"
#include "compiler/dimensions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper: convert nova_location_t to nova_span_t
static inline nova_span_t location_to_span(nova_location_t loc) {
  nova_span_t span = {0};
  span.line = loc.line;
  span.col = loc.column;
  return span;
}

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

static void *ast_malloc(size_t size) {
  void *ptr = calloc(1, size);
  if (!ptr) {
    fprintf(stderr, "AST: Out of memory\n");
    exit(1);
  }
  return ptr;
}

static char *ast_strdup(const char *str) {
  if (!str)
    return NULL;
  char *dup = strdup(str);
  if (!dup) {
    fprintf(stderr, "AST: Out of memory for string\n");
    exit(1);
  }
  return dup;
}

// ══════════════════════════════════════════════════════════════════════════════
// LOCATION HELPERS
// ══════════════════════════════════════════════════════════════════════════════

nova_location_t nova_location(int line, int column) {
  return (nova_location_t){line, column};
}

nova_span_t nova_span_from_location(nova_location_t loc) {
  return location_to_span(loc);
}

// ══════════════════════════════════════════════════════════════════════════════
// TYPE CONSTRUCTORS (for borrow checker)
// ══════════════════════════════════════════════════════════════════════════════

nova_type_t *nova_type_void(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_VOID;
  return type;
}

nova_type_t *nova_type_i32(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_I32;
  return type;
}

nova_type_t *nova_type_str(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_STR;
  return type;
}

nova_type_t *nova_type_ptr(nova_type_t *pointee) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_PTR;
  type->data.ptr.pointee = pointee;
  return type;
}

nova_type_t *nova_type_ptr_mut(nova_type_t *pointee) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_PTR_MUT;
  type->data.ptr.pointee = pointee;
  return type;
}

nova_type_t *nova_type_data(const char *name) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_DATA;
  type->data.name = ast_strdup(name);
  return type;
}

// ══════════════════════════════════════════════════════════════════════════════
// EXPRESSION CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

nova_expr_t *nova_expr_lit_int(int64_t value, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_INT;
  expr->span = location_to_span(loc);
  expr->data.lit_int = value;
  return expr;
}

nova_expr_t *nova_expr_lit_float(double value, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_FLOAT;
  expr->span = location_to_span(loc);
  expr->data.lit_float = value;
  return expr;
}

nova_expr_t *nova_expr_lit_str(const char *value, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_STR;
  expr->span = location_to_span(loc);
  expr->data.lit_str = ast_strdup(value);
  return expr;
}

nova_expr_t *nova_expr_lit_bool(bool value, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_BOOL;
  expr->span = location_to_span(loc);
  expr->data.lit_bool = value;
  return expr;
}

nova_expr_t *nova_expr_ident(const char *name, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_IDENT;
  expr->span = location_to_span(loc);
  expr->data.ident = ast_strdup(name);
  return expr;
}

nova_expr_t *nova_expr_assign(const char *name, nova_expr_t *value,
                              nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_ASSIGN;
  expr->span = location_to_span(loc);
  expr->data.assign.name = ast_strdup(name);
  expr->data.assign.value = value;
  return expr;
}

nova_expr_t *nova_expr_heap_new(nova_type_t *type, nova_expr_t **args,
                                size_t arg_count, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_HEAP_NEW;
  expr->span = location_to_span(loc);
  expr->data.heap_new.type = type;
  expr->data.heap_new.args = args;
  expr->data.heap_new.arg_count = arg_count;
  return expr;
}

nova_expr_t *nova_expr_addr_of(nova_expr_t *expr, nova_location_t loc) {
  nova_expr_t *addr_expr = ast_malloc(sizeof(nova_expr_t));
  addr_expr->kind = EXPR_ADDR_OF;
  addr_expr->span = location_to_span(loc);
  addr_expr->data.addr_of = expr;
  return addr_expr;
}

nova_expr_t *nova_expr_deref(nova_expr_t *expr, nova_location_t loc) {
  nova_expr_t *deref_expr = ast_malloc(sizeof(nova_expr_t));
  deref_expr->kind = EXPR_DEREF;
  deref_expr->span = location_to_span(loc);
  deref_expr->data.deref = expr;
  return deref_expr;
}

nova_expr_t *nova_expr_field_access(nova_expr_t *object, const char *field,
                                    nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_FIELD_ACCESS;
  expr->span = location_to_span(loc);
  expr->data.field_access.object = object;
  expr->data.field_access.field = ast_strdup(field);
  return expr;
}

nova_expr_t *nova_expr_struct_init(const char *struct_name,
                                   nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_STRUCT_INIT;
  expr->span = location_to_span(loc);
  expr->data.struct_init.struct_name = ast_strdup(struct_name);
  expr->data.struct_init.fields = NULL;
  expr->data.struct_init.field_count = 0;
  return expr;
}

nova_expr_t *nova_expr_string_len(nova_expr_t *string, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_STRING_LEN;
  expr->span = location_to_span(loc);
  expr->data.string_len = string;
  return expr;
}

nova_expr_t *nova_expr_string_slice(nova_expr_t *string, nova_expr_t *start,
                                    nova_expr_t *end, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_STRING_SLICE;
  expr->span = location_to_span(loc);
  expr->data.string_slice.string = string;
  expr->data.string_slice.start = start;
  expr->data.string_slice.end = end;
  return expr;
}

nova_expr_t *nova_expr_string_concat(nova_expr_t *left, nova_expr_t *right,
                                     nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_STRING_CONCAT;
  expr->span = location_to_span(loc);
  expr->data.string_concat.left = left;
  expr->data.string_concat.right = right;
  return expr;
}

nova_expr_t *nova_expr_cast(nova_expr_t *expr, nova_type_t *target_type,
                            nova_location_t loc) {
  nova_expr_t *cast_expr = ast_malloc(sizeof(nova_expr_t));
  cast_expr->kind = EXPR_CAST;
  cast_expr->span = location_to_span(loc);
  cast_expr->data.cast.expr = expr;
  cast_expr->data.cast.target_type = target_type;
  return cast_expr;
}

nova_expr_t *nova_expr_namespaced_access(nova_expr_t *object,
                                         const char *member,
                                         nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_NAMESPACED_ACCESS;
  expr->span = location_to_span(loc);
  expr->data.namespaced_access.object = object;
  expr->data.namespaced_access.member = ast_strdup(member);
  return expr;
}

nova_expr_t *nova_expr_call(nova_expr_t *callee, nova_expr_t **args,
                            size_t arg_count, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_CALL;
  expr->span = location_to_span(loc);
  expr->data.call.func = callee;
  expr->data.call.args = args;
  expr->data.call.arg_count = arg_count;
  return expr;
}

nova_expr_t *nova_expr_array_lit(nova_expr_t **elements, size_t count,
                                 nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_ARRAY_LIT;
  expr->span = location_to_span(loc);
  expr->data.array_lit.elements = elements;
  expr->data.array_lit.count = count;
  return expr;
}

nova_expr_t *nova_expr_index(nova_expr_t *object, nova_expr_t *index,
                             nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_INDEX;
  expr->span = location_to_span(loc);
  expr->data.index.object = object;
  expr->data.index.index = index;
  return expr;
}

nova_expr_t *nova_expr_binary(nova_expr_t *left, const char *op,
                              nova_expr_t *right, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_BINARY;
  expr->span = location_to_span(loc);
  expr->data.binary.left = left;
  expr->data.binary.op = ast_strdup(op);
  expr->data.binary.right = right;
  return expr;
}

nova_expr_t *nova_expr_match(nova_expr_t *target, nova_match_arm_t **arms,
                             size_t arm_count, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_MATCH;
  expr->span = location_to_span(loc);
  expr->data.match.target = target;
  expr->data.match.arms = arms;
  expr->data.match.arm_count = arm_count;
  return expr;
}

nova_expr_t *nova_expr_block(nova_stmt_t *block, nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_BLOCK;
  expr->span = location_to_span(loc);
  expr->data.block = block;
  return expr;
}

nova_expr_t *nova_expr_enum_variant(const char *enum_name,
                                    const char *variant_name,
                                    nova_expr_t **args, size_t arg_count,
                                    nova_location_t loc) {
  nova_expr_t *expr = ast_malloc(sizeof(nova_expr_t));
  expr->kind = EXPR_ENUM_VARIANT;
  expr->span = location_to_span(loc);
  expr->data.enum_variant.enum_name = strdup(enum_name);
  expr->data.enum_variant.variant_name = strdup(variant_name);
  expr->data.enum_variant.args = args;
  expr->data.enum_variant.arg_count = arg_count;
  return expr;
}

nova_expr_t *nova_expr_assert(nova_expr_t *condition, nova_expr_t *message,
                              nova_location_t loc) {
  nova_expr_t *expr = malloc(sizeof(nova_expr_t));
  if (!expr)
    return NULL;
  memset(expr, 0, sizeof(nova_expr_t));
  expr->kind = EXPR_ASSERT;
  expr->span.line = loc.line;
  expr->span.col = loc.column;
  expr->data.assert.cond = condition;
  expr->data.assert.message = message;
  return expr;
}

// ══════════════════════════════════════════════════════════════════════════════
// PATTERN CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

nova_pattern_t *nova_pattern_any(nova_location_t loc) {
  nova_pattern_t *pat = ast_malloc(sizeof(nova_pattern_t));
  pat->kind = PATTERN_ANY;
  pat->span = location_to_span(loc);
  return pat;
}

nova_pattern_t *nova_pattern_literal(nova_expr_t *literal,
                                     nova_location_t loc) {
  nova_pattern_t *pat = ast_malloc(sizeof(nova_pattern_t));
  pat->kind = PATTERN_LITERAL;
  pat->span = location_to_span(loc);
  pat->data.literal = literal;
  return pat;
}

nova_pattern_t *nova_pattern_ident(const char *ident, nova_location_t loc) {
  nova_pattern_t *pat = ast_malloc(sizeof(nova_pattern_t));
  pat->kind = PATTERN_IDENT;
  pat->span = location_to_span(loc);
  pat->data.ident = ast_strdup(ident);
  return pat;
}

nova_pattern_t *nova_pattern_variant(const char *name, nova_pattern_t **params,
                                     size_t param_count, nova_location_t loc) {
  nova_pattern_t *pat = ast_malloc(sizeof(nova_pattern_t));
  pat->kind = PATTERN_ENUM;
  pat->span = location_to_span(loc);
  pat->data.variant.name = ast_strdup(name);
  pat->data.variant.params = params;
  pat->data.variant.param_count = param_count;
  return pat;
}

nova_match_arm_t *nova_match_arm(nova_pattern_t *pattern, nova_expr_t *body) {
  nova_match_arm_t *arm = ast_malloc(sizeof(nova_match_arm_t));
  arm->pattern = pattern;
  arm->body = body;
  return arm;
}

// ══════════════════════════════════════════════════════════════════════════════
// STATEMENT CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

nova_stmt_t *nova_stmt_expr(nova_expr_t *expr, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_EXPR;
  stmt->span = location_to_span(loc);
  stmt->data.expr = expr;
  return stmt;
}

nova_stmt_t *nova_stmt_var(const char *name, nova_type_t *type,
                           nova_expr_t *init, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_VAR_DECL;
  stmt->span = location_to_span(loc);
  stmt->data.var_decl.name = ast_strdup(name);
  stmt->data.var_decl.type = type;
  stmt->data.var_decl.init = init;
  return stmt;
}

nova_stmt_t *nova_stmt_heap_free(nova_expr_t *ptr, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_HEAP_FREE;
  stmt->span = location_to_span(loc);
  stmt->data.heap_free = ptr;
  return stmt;
}

nova_stmt_t *nova_stmt_struct_decl(const char *name, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_STRUCT_DECL;
  stmt->span = location_to_span(loc);
  stmt->data.struct_decl = ast_malloc(sizeof(nova_struct_decl_t));
  stmt->data.struct_decl->name = ast_strdup(name);
  stmt->data.struct_decl->fields = NULL;
  stmt->data.struct_decl->field_count = 0;
  return stmt;
}

nova_stmt_t *nova_stmt_block(nova_stmt_t **stmts, size_t count,
                             nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_BLOCK;
  stmt->span = location_to_span(loc);
  stmt->data.block.statements = stmts;
  stmt->data.block.count = count;
  return stmt;
}

nova_stmt_t *nova_stmt_for(const char *binding, nova_expr_t *start,
                           nova_expr_t *end, nova_stmt_t *body,
                           nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_FOR;
  stmt->span = location_to_span(loc);
  stmt->data.for_stmt.binding = ast_strdup(binding);
  stmt->data.for_stmt.start = start;
  stmt->data.for_stmt.end = end;
  stmt->data.for_stmt.body = body;
  return stmt;
}

nova_stmt_t *nova_stmt_enum_decl(const char *name, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_ENUM_DECL;
  stmt->span = location_to_span(loc);
  stmt->data.enum_decl.name = ast_strdup(name);
  stmt->data.enum_decl.variants = NULL;
  stmt->data.enum_decl.variant_count = 0;
  return stmt;
}

nova_stmt_t *nova_stmt_try_catch(nova_stmt_t *try_block,
                                 nova_stmt_t *catch_block,
                                 const char *exception_name,
                                 nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_TRY_CATCH;
  stmt->span = location_to_span(loc);
  stmt->data.try_catch_stmt.try_block = try_block;
  stmt->data.try_catch_stmt.catch_block = catch_block;
  stmt->data.try_catch_stmt.exception_name =
      exception_name ? ast_strdup(exception_name) : NULL;
  return stmt;
}

// Forward declaration for deep expr clone used by clone_contracts
static nova_expr_t *nova_expr_clone(nova_expr_t *expr);

// Helper: clone contract statements (deep)
static nova_contract_stmt_t *clone_contracts(nova_contract_stmt_t *src,
                                             size_t count) {
  if (count == 0 || !src)
    return NULL;
  nova_contract_stmt_t *dst = ast_malloc(sizeof(nova_contract_stmt_t) * count);
  for (size_t i = 0; i < count; i++) {
    /* FIX: deep-clone each expression so ownership is unambiguous */
    dst[i].cond    = nova_expr_clone(src[i].cond);
    dst[i].message = src[i].message ? nova_expr_clone(src[i].message) : NULL;
  }
  return dst;
}

/*
 * nova_expr_clone — minimal deep copy sufficient for contract cloning.
 * Covers literal leaves and the EXPR_CALL / EXPR_BINARY / EXPR_IDENT cases
 * that appear in requires/ensures clauses.  Other nodes fall through to a
 * shallow copy which is safe as long as the original AST outlives the clone.
 */
static nova_expr_t *nova_expr_clone(nova_expr_t *expr) {
  if (!expr) return NULL;
  nova_expr_t *c = ast_malloc(sizeof(nova_expr_t));
  *c = *expr; /* shallow baseline */
  c->type = expr->type ? nova_type_clone(expr->type) : NULL;

  switch (expr->kind) {
  case EXPR_INT: case EXPR_FLOAT: case EXPR_BOOL: break; /* scalars — done */
  case EXPR_STR:   c->data.lit_str = expr->data.lit_str ? ast_strdup(expr->data.lit_str) : NULL; break;
  case EXPR_IDENT: c->data.ident   = expr->data.ident   ? ast_strdup(expr->data.ident)   : NULL; break;
  case EXPR_BINARY:
    c->data.binary.op    = expr->data.binary.op ? ast_strdup(expr->data.binary.op) : NULL;
    c->data.binary.left  = nova_expr_clone(expr->data.binary.left);
    c->data.binary.right = nova_expr_clone(expr->data.binary.right);
    break;
  case EXPR_CALL: {
    c->data.call.func = nova_expr_clone(expr->data.call.func);
    size_t n = expr->data.call.arg_count;
    if (n > 0) {
      c->data.call.args = ast_malloc(sizeof(nova_expr_t *) * n);
      for (size_t i = 0; i < n; i++)
        c->data.call.args[i] = nova_expr_clone(expr->data.call.args[i]);
    }
    break;
  }
  default:
    /* For complex nodes in contracts (rare): shallow copy is acceptable. */
    break;
  }
  return c;
}

// ══════════════════════════════════════════════════════════════════════════════
// PROGRAM CONSTRUCTORS
// ══════════════════════════════════════════════════════════════════════════════

nova_program_t *nova_program_create(const char *filename) {
  nova_program_t *program = ast_malloc(sizeof(nova_program_t));
  program->filename = ast_strdup(filename);
  program->declarations = NULL;
  program->declaration_count = 0;
  return program;
}

void nova_program_add_decl(nova_program_t *program, nova_top_level_t *decl) {
  program->declaration_count++;
  program->declarations =
      realloc(program->declarations,
              sizeof(nova_top_level_t *) * program->declaration_count);
  program->declarations[program->declaration_count - 1] = decl;
}

// ══════════════════════════════════════════════════════════════════════════════
// MEMORY MANAGEMENT
// ══════════════════════════════════════════════════════════════════════════════

void nova_type_free(nova_type_t *type) {
  if (!type)
    return;

  switch (type->kind) {
  case TYPE_PTR:
  case TYPE_PTR_MUT:
  case TYPE_ARRAY:
  case TYPE_POINTER:
    nova_type_free(type->data.ptr.pointee);
    break;
  case TYPE_FN:
    for (size_t i = 0; i < type->data.fn.param_count; i++) {
      nova_type_free(type->data.fn.params[i]);
    }
    if (type->data.fn.params)
      free(type->data.fn.params);
    nova_type_free(type->data.fn.return_type);
    break;
  case TYPE_DATA:
    if (type->data.name)
      free(type->data.name);
    break;
  case TYPE_QTY:
    nova_type_free(type->data.qty.inner_type);
    if (type->data.qty.unit_expr)
      free(type->data.qty.unit_expr);
    if (type->data.qty.dimension) {
      nova_dim_destroy((nova_dimension_t *)type->data.qty.dimension);
    }
    break;
  case TYPE_FLOW:
    nova_type_free(type->data.flow.inner);
    break;
  case TYPE_VAR:
    if (type->data.var_name)
      free(type->data.var_name);
    break;
  case TYPE_GENERIC:
    if (type->data.generic.name)
      free(type->data.generic.name);
    if (type->data.generic.args && type->data.generic.arg_count > 0) {
      for (size_t i = 0; i < type->data.generic.arg_count; i++)
        nova_type_free(type->data.generic.args[i]);
      free(type->data.generic.args);
    }
    break;
  case TYPE_TUPLE:
    if (type->data.tuple.elements && type->data.tuple.count > 0) {
      for (size_t i = 0; i < type->data.tuple.count; i++)
        nova_type_free(type->data.tuple.elements[i]);
      free(type->data.tuple.elements);
    }
    break;
  case TYPE_CONTRACT:
    /* FIX: free the base type and contract expression arrays */
    nova_type_free(type->data.contract.base_type);
    if (type->data.contract.requires) {
      for (size_t i = 0; i < type->data.contract.num_requires; i++)
        nova_expr_free(type->data.contract.requires[i]);
      free(type->data.contract.requires);
    }
    if (type->data.contract.ensures) {
      for (size_t i = 0; i < type->data.contract.num_ensures; i++)
        nova_expr_free(type->data.contract.ensures[i]);
      free(type->data.contract.ensures);
    }
    break;
  case TYPE_DEPENDENT_PI:
  case TYPE_EFFECT:
    /* No heap sub-fields defined yet in ast.h — safe no-op */
    break;
  default:
    break;
  }

  free(type);
}

void nova_pattern_free(nova_pattern_t *pat) {
  if (!pat)
    return;
  switch (pat->kind) {
  case PATTERN_LITERAL:
    nova_expr_free(pat->data.literal);
    break;
  case PATTERN_IDENT:
    free(pat->data.ident);
    break;
  case PATTERN_ENUM:
    free(pat->data.variant.name);
    for (size_t i = 0; i < pat->data.variant.param_count; i++) {
      nova_pattern_free(pat->data.variant.params[i]);
    }
    free(pat->data.variant.params);
    break;
  default:
    break;
  }
  free(pat);
}

void nova_match_arm_free(nova_match_arm_t *arm) {
  if (!arm)
    return;
  nova_pattern_free(arm->pattern);
  nova_expr_free(arm->body);
  free(arm);
}

void nova_expr_free(nova_expr_t *expr) {
  if (!expr)
    return;

  if (expr->type)
    nova_type_free(expr->type);

  switch (expr->kind) {
  case EXPR_STR:
    free(expr->data.lit_str);
    break;
  case EXPR_IDENT:
    free(expr->data.ident);
    break;
  case EXPR_BINARY:
    nova_expr_free(expr->data.binary.left);
    free(expr->data.binary.op);
    nova_expr_free(expr->data.binary.right);
    break;
  case EXPR_ASSIGN:
    free(expr->data.assign.name);
    nova_expr_free(expr->data.assign.value);
    break;
  case EXPR_HEAP_NEW:
    nova_type_free(expr->data.heap_new.type);
    for (size_t i = 0; i < expr->data.heap_new.arg_count; i++) {
      nova_expr_free(expr->data.heap_new.args[i]);
    }
    free(expr->data.heap_new.args);
    break;
  case EXPR_ADDR_OF:
    nova_expr_free(expr->data.addr_of);
    break;
  case EXPR_DEREF:
    nova_expr_free(expr->data.deref);
    break;
  case EXPR_FIELD_ACCESS:
    nova_expr_free(expr->data.field_access.object);
    free(expr->data.field_access.field);
    break;
  case EXPR_STRUCT_INIT:
    free(expr->data.struct_init.struct_name);
    for (size_t i = 0; i < expr->data.struct_init.field_count; i++) {
      free(expr->data.struct_init.fields[i].name);
      nova_expr_free(expr->data.struct_init.fields[i].value);
    }
    free(expr->data.struct_init.fields);
    break;
  case EXPR_STRING_LEN:
    nova_expr_free(expr->data.string_len);
    break;
  case EXPR_STRING_SLICE:
    nova_expr_free(expr->data.string_slice.string);
    nova_expr_free(expr->data.string_slice.start);
    nova_expr_free(expr->data.string_slice.end);
    break;
  case EXPR_STRING_CONCAT:
    nova_expr_free(expr->data.string_concat.left);
    nova_expr_free(expr->data.string_concat.right);
    break;
  case EXPR_CAST:
    nova_expr_free(expr->data.cast.expr);
    nova_type_free(expr->data.cast.target_type);
    break;
  case EXPR_NAMESPACED_ACCESS:
    nova_expr_free(expr->data.namespaced_access.object);
    free(expr->data.namespaced_access.member);
    break;
  case EXPR_ARRAY_LIT:
    for (size_t i = 0; i < expr->data.array_lit.count; i++) {
      nova_expr_free(expr->data.array_lit.elements[i]);
    }
    free(expr->data.array_lit.elements);
    break;
  case EXPR_INDEX:
    nova_expr_free(expr->data.index.object);
    nova_expr_free(expr->data.index.index);
    break;
  case EXPR_MATCH:
    nova_expr_free(expr->data.match.target);
    for (size_t i = 0; i < expr->data.match.arm_count; i++) {
      nova_match_arm_free(expr->data.match.arms[i]);
    }
    free(expr->data.match.arms);
    break;
  case EXPR_ENUM_VARIANT:
    free(expr->data.enum_variant.enum_name);
    free(expr->data.enum_variant.variant_name);
    for (size_t i = 0; i < expr->data.enum_variant.arg_count; i++) {
      nova_expr_free(expr->data.enum_variant.args[i]);
    }
    free(expr->data.enum_variant.args);
    break;
  case EXPR_ASSERT:
    nova_expr_free(expr->data.assert.cond);
    if (expr->data.assert.message)
      nova_expr_free(expr->data.assert.message);
    break;
  case EXPR_BLOCK:
    if (expr->data.block)
      nova_stmt_free(expr->data.block);
    break;
  case EXPR_AWAIT:
    nova_expr_free(expr->data.await_expr);
    break;
  default:
    break;
  }

  free(expr);
}

void nova_stmt_free(nova_stmt_t *stmt) {
  if (!stmt)
    return;

  switch (stmt->kind) {
  case STMT_EXPR:
    if (stmt->data.expr)
      nova_expr_free(stmt->data.expr);
    break;
  case STMT_VAR_DECL:
    if (stmt->data.var_decl.name)
      free(stmt->data.var_decl.name);
    if (stmt->data.var_decl.type)
      nova_type_free(stmt->data.var_decl.type);
    if (stmt->data.var_decl.init)
      nova_expr_free(stmt->data.var_decl.init);
    break;
  case STMT_BLOCK:
    for (size_t i = 0; i < stmt->data.block.count; i++) {
      nova_stmt_free(stmt->data.block.statements[i]);
    }
    if (stmt->data.block.statements)
      free(stmt->data.block.statements);
    break;
  case STMT_FN:
    if (stmt->data.fn_stmt.name)
      free(stmt->data.fn_stmt.name);
    for (size_t i = 0; i < stmt->data.fn_stmt.param_count; i++) {
      if (stmt->data.fn_stmt.params[i]) {
        if (stmt->data.fn_stmt.params[i]->name)
          free(stmt->data.fn_stmt.params[i]->name);
        if (stmt->data.fn_stmt.params[i]->type)
          nova_type_free(stmt->data.fn_stmt.params[i]->type);
        free(stmt->data.fn_stmt.params[i]);
      }
    }
    if (stmt->data.fn_stmt.params)
      free(stmt->data.fn_stmt.params);
    if (stmt->data.fn_stmt.return_type)
      nova_type_free(stmt->data.fn_stmt.return_type);
    for (size_t i = 0; i < stmt->data.fn_stmt.requires_count; i++) {
      nova_expr_free(stmt->data.fn_stmt.requires[i].cond);
      if (stmt->data.fn_stmt.requires[i].message)
        nova_expr_free(stmt->data.fn_stmt.requires[i].message);
    }
    free(stmt->data.fn_stmt.requires);
    for (size_t i = 0; i < stmt->data.fn_stmt.ensures_count; i++) {
      nova_expr_free(stmt->data.fn_stmt.ensures[i].cond);
      if (stmt->data.fn_stmt.ensures[i].message)
        nova_expr_free(stmt->data.fn_stmt.ensures[i].message);
    }
    free(stmt->data.fn_stmt.ensures);
    if (stmt->data.fn_stmt.body)
      nova_stmt_free(stmt->data.fn_stmt.body);
    break;
  case STMT_YIELD:
    if (stmt->data.yield_stmt)
      nova_expr_free(stmt->data.yield_stmt);
    break;
  case STMT_CHECK:
    if (stmt->data.check_stmt.condition)
      nova_expr_free(stmt->data.check_stmt.condition);
    if (stmt->data.check_stmt.then_branch)
      nova_stmt_free(stmt->data.check_stmt.then_branch);
    if (stmt->data.check_stmt.else_branch)
      nova_stmt_free(stmt->data.check_stmt.else_branch);
    break;
  case STMT_WHILE:
    if (stmt->data.while_stmt.condition)
      nova_expr_free(stmt->data.while_stmt.condition);
    if (stmt->data.while_stmt.body)
      nova_stmt_free(stmt->data.while_stmt.body);
    break;
  case STMT_HEAP_FREE:
    if (stmt->data.heap_free)
      nova_expr_free(stmt->data.heap_free);
    break;
  case STMT_STRUCT_DECL:
    if (stmt->data.struct_decl) {
      free(stmt->data.struct_decl->name);
      for (size_t i = 0; i < stmt->data.struct_decl->field_count; i++) {
        free(stmt->data.struct_decl->fields[i].name);
        nova_type_free(stmt->data.struct_decl->fields[i].type);
        // free(stmt->data.struct_decl->fields[i]); // fields is inline struct
        // array
      }
      free(stmt->data.struct_decl->fields);
      free(stmt->data.struct_decl);
    }
    break;
  case STMT_FOR:
    if (stmt->data.for_stmt.binding)
      free(stmt->data.for_stmt.binding);
    if (stmt->data.for_stmt.start)
      nova_expr_free(stmt->data.for_stmt.start);
    if (stmt->data.for_stmt.end)
      nova_expr_free(stmt->data.for_stmt.end);
    if (stmt->data.for_stmt.body)
      nova_stmt_free(stmt->data.for_stmt.body);
    break;
  case STMT_ENUM_DECL:
    free(stmt->data.enum_decl.name);
    for (size_t i = 0; i < stmt->data.enum_decl.variant_count; i++) {
      free(stmt->data.enum_decl.variants[i].name);
      for (size_t j = 0; j < stmt->data.enum_decl.variants[i].field_count;
           j++) {
        nova_type_free(stmt->data.enum_decl.variants[i].field_types[j]);
      }
      free(stmt->data.enum_decl.variants[i].field_types);
    }
    free(stmt->data.enum_decl.variants);
    break;
  case STMT_TRY_CATCH:
    if (stmt->data.try_catch_stmt.try_block)
      nova_stmt_free(stmt->data.try_catch_stmt.try_block);
    if (stmt->data.try_catch_stmt.catch_block)
      nova_stmt_free(stmt->data.try_catch_stmt.catch_block);
    if (stmt->data.try_catch_stmt.exception_name)
      free(stmt->data.try_catch_stmt.exception_name);
    break;
  case STMT_EACH:
    if (stmt->data.each_stmt.binding)
      free(stmt->data.each_stmt.binding);
    if (stmt->data.each_stmt.iterator)
      nova_expr_free(stmt->data.each_stmt.iterator);
    if (stmt->data.each_stmt.body)
      nova_stmt_free(stmt->data.each_stmt.body);
    break;
  case STMT_SPAWN:
    if (stmt->data.spawn_stmt.expr)
      nova_expr_free(stmt->data.spawn_stmt.expr);
    break;
  case STMT_FOREIGN:
    if (stmt->data.foreign_stmt.abi)
      free(stmt->data.foreign_stmt.abi);
    if (stmt->data.foreign_stmt.body)
      nova_stmt_free(stmt->data.foreign_stmt.body);
    break;
  default:
    break;
  }

  free(stmt);
}

void nova_program_free(nova_program_t *program) {
  if (!program)
    return;

  /* FIX: free each declaration's data before freeing the pointer array.
   * Previously, free(program->declarations) only freed the pointer array
   * itself — all nova_stmt_t / nova_struct_decl_t payloads were leaked. */
  for (size_t i = 0; i < program->declaration_count; i++) {
    nova_top_level_t *decl = program->declarations[i];
    if (!decl) continue;
    if (decl->kind == DECL_FUNCTION && decl->data)
      nova_stmt_free((nova_stmt_t *)decl->data);
    /* DECL_STRUCT, DECL_ENUM etc. — add here when struct_decl_free exists */
    free(decl);
  }

  free(program->declarations);
  free(program->filename);
  free(program);
}

// ══════════════════════════════════════════════════════════════════════════════
// DEBUG PRINTING
// ══════════════════════════════════════════════════════════════════════════════

void nova_type_print(nova_type_t *type) {
  if (!type) {
    printf("<null>");
    return;
  }

  switch (type->kind) {
  case TYPE_VOID:
    printf("void");
    break;
  case TYPE_BOOL:
    printf("bool");
    break;
  case TYPE_I32:
    printf("i32");
    break;
  case TYPE_I64:
    printf("i64");
    break;
  case TYPE_F32:
    printf("f32");
    break;
  case TYPE_F64:
    printf("f64");
    break;
  case TYPE_USIZE:
    printf("usize");
    break;
  case TYPE_U8:
    printf("u8");
    break;
  case TYPE_STR:
    printf("str");
    break;
  case TYPE_PTR:
    nova_type_print(type->data.ptr.pointee);
    printf("*");
    break;
  case TYPE_ARRAY:
    printf("[");
    nova_type_print(type->data.ptr.pointee);
    if (type->data.ptr.size > 0) {
      printf("; %zu", type->data.ptr.size);
    }
    printf("]");
    break;
  case TYPE_FN:
    printf("fn(");
    for (size_t i = 0; i < type->data.fn.param_count; i++) {
      if (i > 0)
        printf(", ");
      nova_type_print(type->data.fn.params[i]);
    }
    printf(") -> ");
    nova_type_print(type->data.fn.return_type);
    break;
  case TYPE_DATA:
    printf("%s", type->data.name);
    break;
  case TYPE_POINTER:
    printf("*");
    nova_type_print(type->data.ptr.pointee);
    break;
  default:
    printf("<unknown>");
    break;
  }
}

void nova_expr_print(nova_expr_t *expr, int indent) {
  for (int i = 0; i < indent; i++)
    fprintf(stderr, "  ");

  if (!expr) {
    fprintf(stderr, "<null>\n");
    return;
  }

  switch (expr->kind) {
  case EXPR_INT:
    fprintf(stderr, "%lld\n", expr->data.lit_int);
    break;
  case EXPR_FLOAT:
    fprintf(stderr, "%f\n", expr->data.lit_float);
    break;
  case EXPR_STR:
    fprintf(stderr, "\"%s\"\n", expr->data.lit_str);
    break;
  case EXPR_BOOL:
    fprintf(stderr, "%s\n", expr->data.lit_bool ? "true" : "false");
    break;
  case EXPR_IDENT:
    fprintf(stderr, "%s\n", expr->data.ident);
    break;
  case EXPR_BINARY:
    fprintf(stderr, "(%s\n", expr->data.binary.op);
    nova_expr_print(expr->data.binary.left, indent + 1);
    nova_expr_print(expr->data.binary.right, indent + 1);
    for (int i = 0; i < indent; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, ")\n");
    break;
  case EXPR_ASSIGN:
    fprintf(stderr, "(assign\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "target: %s\n", expr->data.assign.name);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "value:\n");
    nova_expr_print(expr->data.assign.value, indent + 2);
    for (int i = 0; i < indent; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, ")\n");
    break;
  case EXPR_CALL:
    fprintf(stderr, "call\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "callee:\n");
    nova_expr_print(expr->data.call.func, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "args(%zu):\n", expr->data.call.arg_count);
    for (size_t i = 0; i < expr->data.call.arg_count; i++) {
      nova_expr_print(expr->data.call.args[i], indent + 2);
    }
    break;
  case EXPR_HEAP_NEW:
    fprintf(stderr, "heap_new\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "type: ");
    nova_type_print(expr->data.heap_new.type);
    fprintf(stderr, "\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "args(%zu):\n", expr->data.heap_new.arg_count);
    for (size_t i = 0; i < expr->data.heap_new.arg_count; i++) {
      nova_expr_print(expr->data.heap_new.args[i], indent + 2);
    }
    break;
  case EXPR_ADDR_OF:
    fprintf(stderr, "addr_of\n");
    nova_expr_print(expr->data.addr_of, indent + 1);
    break;
  case EXPR_DEREF:
    fprintf(stderr, "deref\n");
    nova_expr_print(expr->data.deref, indent + 1);
    break;
  case EXPR_FIELD_ACCESS:
    fprintf(stderr, "field_access\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "object:\n");
    nova_expr_print(expr->data.field_access.object, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "field: %s\n", expr->data.field_access.field);
    break;
  case EXPR_STRUCT_INIT:
    fprintf(stderr, "struct_init %s\n", expr->data.struct_init.struct_name);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "fields(%zu):\n", expr->data.struct_init.field_count);
    for (size_t i = 0; i < expr->data.struct_init.field_count; i++) {
      for (int j = 0; j < indent + 2; j++)
        fprintf(stderr, "  ");
      fprintf(stderr, "%s =\n", expr->data.struct_init.fields[i].name);
      nova_expr_print(expr->data.struct_init.fields[i].value, indent + 3);
    }
    break;
  case EXPR_STRING_LEN:
    fprintf(stderr, "string_len\n");
    nova_expr_print(expr->data.string_len, indent + 1);
    break;
  case EXPR_STRING_SLICE:
    fprintf(stderr, "string_slice\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "string:\n");
    nova_expr_print(expr->data.string_slice.string, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "start:\n");
    nova_expr_print(expr->data.string_slice.start, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "end:\n");
    nova_expr_print(expr->data.string_slice.end, indent + 2);
    break;
  case EXPR_STRING_CONCAT:
    fprintf(stderr, "string_concat\n");
    nova_expr_print(expr->data.string_concat.left, indent + 1);
    nova_expr_print(expr->data.string_concat.right, indent + 1);
    break;
  case EXPR_CAST:
    fprintf(stderr, "cast\n");
    nova_expr_print(expr->data.cast.expr, indent + 1);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "to: ");
    nova_type_print(expr->data.cast.target_type);
    fprintf(stderr, "\n");
    break;
  case EXPR_NAMESPACED_ACCESS:
    fprintf(stderr, "namespaced_access\n");
    nova_expr_print(expr->data.namespaced_access.object, indent + 1);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "member: %s\n", expr->data.namespaced_access.member);
    break;
  case EXPR_AWAIT:
    fprintf(stderr, "await\n");
    nova_expr_print(expr->data.await_expr, indent + 1);
    break;
  default:
    fprintf(stderr, "<unknown expr %d>\n", expr->kind);
    break;
  }
}

void nova_stmt_print(nova_stmt_t *stmt, int indent) {
  for (int i = 0; i < indent; i++)
    fprintf(stderr, "  ");

  if (!stmt) {
    fprintf(stderr, "<null>\n");
    return;
  }

  switch (stmt->kind) {
  case STMT_EXPR:
    fprintf(stderr, "expr\n");
    nova_expr_print(stmt->data.expr, indent + 1);
    break;
  case STMT_VAR_DECL:
    fprintf(stderr, "var %s", stmt->data.var_decl.name);
    if (stmt->data.var_decl.type) {
      fprintf(stderr, ": ");
      nova_type_print(stmt->data.var_decl.type);
    }
    if (stmt->data.var_decl.init) {
      fprintf(stderr, " =\n");
      nova_expr_print(stmt->data.var_decl.init, indent + 1);
    } else {
      fprintf(stderr, "\n");
    }
    break;
  case STMT_CHECK:
    fprintf(stderr, "check\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "condition:\n");
    nova_expr_print(stmt->data.check_stmt.condition, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "then:\n");
    nova_stmt_print(stmt->data.check_stmt.then_branch, indent + 2);
    if (stmt->data.check_stmt.else_branch) {
      for (int i = 0; i < indent + 1; i++)
        fprintf(stderr, "  ");
      fprintf(stderr, "else:\n");
      nova_stmt_print(stmt->data.check_stmt.else_branch, indent + 2);
    }
    break;
  case STMT_YIELD:
    fprintf(stderr, "yield");
    if (stmt->data.yield_stmt) {
      fprintf(stderr, "\n");
      nova_expr_print(stmt->data.yield_stmt, indent + 1);
    } else {
      fprintf(stderr, "\n");
    }
    break;
  case STMT_BREAK:
    fprintf(stderr, "break\n");
    break;
  case STMT_CONTINUE:
    fprintf(stderr, "continue\n");
    break;
  case STMT_FN:
    fprintf(stderr, "fn %s(",
            stmt->data.fn_stmt.name ? stmt->data.fn_stmt.name : "<anonymous>");
    for (size_t i = 0; i < stmt->data.fn_stmt.param_count; i++) {
      if (i > 0)
        fprintf(stderr, ", ");
      fprintf(stderr, "%s", stmt->data.fn_stmt.params[i]->name);
    }
    fprintf(stderr, ")\n");
    if (stmt->data.fn_stmt.body) {
      nova_stmt_print(stmt->data.fn_stmt.body, indent + 1);
    }
    break;
  case STMT_BLOCK:
    fprintf(stderr, "block (%zu statements)\n", stmt->data.block.count);
    for (size_t i = 0; i < stmt->data.block.count; i++) {
      nova_stmt_print(stmt->data.block.statements[i], indent + 1);
    }
    break;
  case STMT_WHILE:
    fprintf(stderr, "while\n");
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "condition:\n");
    nova_expr_print(stmt->data.while_stmt.condition, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      fprintf(stderr, "  ");
    fprintf(stderr, "body:\n");
    nova_stmt_print(stmt->data.while_stmt.body, indent + 2);
    break;
  case STMT_HEAP_FREE:
    fprintf(stderr, "heap_free\n");
    nova_expr_print(stmt->data.heap_free, indent + 1);
    break;
  case STMT_STRUCT_DECL:
    fprintf(stderr, "struct_decl %s\n", stmt->data.struct_decl->name);
    for (size_t i = 0; i < stmt->data.struct_decl->field_count; i++) {
      for (int j = 0; j < indent + 1; j++)
        fprintf(stderr, "  ");
      fprintf(stderr, "field %s: ", stmt->data.struct_decl->fields[i].name);
      nova_type_print(stmt->data.struct_decl->fields[i].type);
      fprintf(stderr, "\n");
    }
    break;
  case STMT_EACH:
    fprintf(stderr, "each %s in\n", stmt->data.each_stmt.binding);
    nova_expr_print(stmt->data.each_stmt.iterator, indent + 1);
    nova_stmt_print(stmt->data.each_stmt.body, indent + 1);
    break;
  case STMT_SPAWN:
    fprintf(stderr, "spawn\n");
    nova_expr_print(stmt->data.spawn_stmt.expr, indent + 1);
    break;
  case STMT_FOREIGN:
    fprintf(stderr, "foreign \"%s\"\n", stmt->data.foreign_stmt.abi);
    nova_stmt_print(stmt->data.foreign_stmt.body, indent + 1);
    break;
  default:
    fprintf(stderr, "<unknown stmt %d>\n", stmt->kind);
    break;
  }
}

void nova_program_print(nova_program_t *program) {
  printf("Program: %s\n", program->filename);
  printf("Declarations: %zu\n", program->declaration_count);
}

// Additional type constructors
nova_type_t *nova_type_bool(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_BOOL;
  return type;
}

nova_type_t *nova_type_i64(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_I64;
  return type;
}

nova_type_t *nova_type_f32(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_F32;
  return type;
}

nova_type_t *nova_type_f64(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_F64;
  return type;
}

nova_type_t *nova_type_u8(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_U8;
  return type;
}

/* Create quantity type for unit algebra */
nova_expr_t *nova_expr_unit_literal(double value, const char *unit,
                                    nova_location_t loc) {
  nova_expr_t *expr = malloc(sizeof(nova_expr_t));
  if (!expr)
    return NULL;
  memset(expr, 0, sizeof(nova_expr_t));
  expr->kind = EXPR_UNIT_LITERAL;
  expr->span = nova_span_from_location(loc);
  expr->data.unit_literal.value = value;
  expr->data.unit_literal.unit = unit ? strdup(unit) : NULL;
  expr->data.unit_literal.dimension = NULL; /* filled by semantic analysis */
  return expr;
}

/* ── nova_type_clone ─────────────────────────────────────────────────────────
 * Public deep-copy function (declared in ast.h, used by semantic,
 * module_registry, etc.)
 */
nova_type_t *nova_type_clone(const nova_type_t *type) {
  if (!type)
    return NULL;

  nova_type_t *copy = calloc(1, sizeof(nova_type_t));
  if (!copy)
    return NULL;

  copy->kind = type->kind;

  switch (type->kind) {
  case TYPE_PTR:
  case TYPE_PTR_MUT:
  case TYPE_ARRAY:
  case TYPE_POINTER:
    copy->data.ptr.pointee = nova_type_clone(type->data.ptr.pointee);
    copy->data.ptr.size = (type->kind == TYPE_ARRAY) ? type->data.ptr.size : 0;
    break;
  case TYPE_DATA:
    copy->data.name = type->data.name ? strdup(type->data.name) : NULL;
    break;
  case TYPE_QTY:
    copy->data.qty.inner_type = nova_type_clone(type->data.qty.inner_type);
    copy->data.qty.unit_expr =
        type->data.qty.unit_expr ? strdup(type->data.qty.unit_expr) : NULL;
    copy->data.qty.dimension = type->data.qty.unit_expr
                                   ? nova_dim_parse(type->data.qty.unit_expr)
                                   : NULL;
    break;
  case TYPE_FN:
    copy->data.fn.param_count = type->data.fn.param_count;
    if (type->data.fn.params && type->data.fn.param_count > 0) {
      copy->data.fn.params =
          calloc(type->data.fn.param_count, sizeof(nova_type_t *));
      for (size_t i = 0; i < type->data.fn.param_count; i++)
        copy->data.fn.params[i] = nova_type_clone(type->data.fn.params[i]);
    } else {
      copy->data.fn.params = NULL;
    }
    copy->data.fn.return_type = nova_type_clone(type->data.fn.return_type);
    break;
  case TYPE_FLOW:
    copy->data.flow.flow_kind = type->data.flow.flow_kind;
    copy->data.flow.inner = nova_type_clone(type->data.flow.inner);
    break;
  case TYPE_VAR:
    copy->data.var_name =
        type->data.var_name ? strdup(type->data.var_name) : NULL;
    break;
  case TYPE_GENERIC:
    copy->data.generic.name =
        type->data.generic.name ? strdup(type->data.generic.name) : NULL;
    copy->data.generic.arg_count = type->data.generic.arg_count;
    if (type->data.generic.args && type->data.generic.arg_count > 0) {
      copy->data.generic.args =
          calloc(type->data.generic.arg_count, sizeof(nova_type_t *));
      for (size_t i = 0; i < type->data.generic.arg_count; i++)
        copy->data.generic.args[i] =
            nova_type_clone(type->data.generic.args[i]);
    }
    break;
  case TYPE_TUPLE:
    copy->data.tuple.count = type->data.tuple.count;
    if (type->data.tuple.elements && type->data.tuple.count > 0) {
      copy->data.tuple.elements =
          calloc(type->data.tuple.count, sizeof(nova_type_t *));
      for (size_t i = 0; i < type->data.tuple.count; i++)
        copy->data.tuple.elements[i] =
            nova_type_clone(type->data.tuple.elements[i]);
    }
    break;
  default:
    /* Basic types like I32, F64 are already handled by kind assignment */
    break;
  }
  return copy;
}

nova_type_t *nova_type_qty(nova_type_t *base_type, const char *unit_expr) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_QTY;
  type->data.qty.inner_type = base_type;
  type->data.qty.unit_expr = unit_expr ? strdup(unit_expr) : NULL;
  type->data.qty.dimension = NULL; // Will be set by semantic analysis
  return type;
}

nova_type_t *nova_type_usize(void) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_USIZE;
  return type;
}

nova_type_t *nova_type_pointer(nova_type_t *pointee) {
  return nova_type_ptr(pointee); // Alias for nova_type_ptr
}

nova_type_t *nova_type_array(nova_type_t *element, size_t size) {
  nova_type_t *type = ast_malloc(sizeof(nova_type_t));
  type->kind = TYPE_ARRAY;
  type->data.ptr.pointee = element;
  type->data.ptr.size = size;
  return type;
}
nova_stmt_t *nova_stmt_each(const char *binding, nova_expr_t *iterator,
                            nova_stmt_t *body, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_EACH;
  stmt->span = location_to_span(loc);
  stmt->data.each_stmt.binding = ast_strdup(binding);
  stmt->data.each_stmt.iterator = iterator;
  stmt->data.each_stmt.body = body;
  return stmt;
}

nova_stmt_t *nova_stmt_spawn(nova_expr_t *expr, nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_SPAWN;
  stmt->span = location_to_span(loc);
  stmt->data.spawn_stmt.expr = expr;
  return stmt;
}

nova_stmt_t *nova_stmt_foreign(const char *abi, nova_stmt_t *body,
                               nova_location_t loc) {
  nova_stmt_t *stmt = ast_malloc(sizeof(nova_stmt_t));
  stmt->kind = STMT_FOREIGN;
  stmt->span = location_to_span(loc);
  stmt->data.foreign_stmt.abi = ast_strdup(abi);
  stmt->data.foreign_stmt.body = body;
  return stmt;
}

nova_expr_t *nova_expr_await(nova_expr_t *expr, nova_location_t loc) {
  nova_expr_t *e = ast_malloc(sizeof(nova_expr_t));
  e->kind = EXPR_AWAIT;
  e->span = location_to_span(loc);
  e->data.await_expr = expr;
  return e;
}

/* ── nova_stmt_var_decl ──────────────────────────────────────────────────────
 * Declared in ast.h alongside nova_stmt_var().  Both produce STMT_VAR_DECL;
 * nova_stmt_var_decl is the canonical name used by the parser and codegen.
 */
nova_stmt_t *nova_stmt_var_decl(const char *name, nova_type_t *type,
                                nova_expr_t *init, nova_location_t loc) {
  return nova_stmt_var(name, type, init, loc);
}
