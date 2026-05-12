/**
 * Nova LLVM IR Code Generator
 *
 * Converts Nova AST to LLVM IR for JIT/AOT compilation
 */

#include "../../../../include/compiler/ast.h"

#include "nova_llvm_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>

// ═══════════════════════════════════════════════════════════════════════════
// Code Generation Context
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaLLVMContext *llvm_ctx;
  LLVMValueRef current_function;

  // Symbol table for variables (name -> LLVMValueRef)
  struct {
    char **names;
    LLVMValueRef *values;
    LLVMTypeRef *types;
    int *scope_depths; // FIX: track which scope each symbol belongs to
    int count;
    int capacity;
  } symbols;

  int current_scope; // FIX: current scope depth

  // Struct types (name -> LLVMTypeRef)
  struct {
    char **names;
    LLVMTypeRef *types;
    char ***field_names;
    int *field_counts;
    int count;
    int capacity;
  } structs;
} CodegenContext;

static CodegenContext *codegen_context_create(NovaLLVMContext *llvm_ctx) {
  CodegenContext *ctx = (CodegenContext *)malloc(sizeof(CodegenContext));
  ctx->llvm_ctx = llvm_ctx;
  ctx->current_function = NULL;
  ctx->symbols.names = NULL;
  ctx->symbols.values = NULL;
  ctx->symbols.types = NULL;
  ctx->symbols.scope_depths = NULL; // FIX
  ctx->symbols.count = 0;
  ctx->symbols.capacity = 0;
  ctx->current_scope = 0; // FIX
  ctx->structs.names = NULL;
  ctx->structs.types = NULL;
  ctx->structs.field_names = NULL;
  ctx->structs.field_counts = NULL;
  ctx->structs.count = 0;
  ctx->structs.capacity = 0;
  return ctx;
}

static void codegen_context_destroy(CodegenContext *ctx) {
  if (!ctx)
    return;

  // FIX: free each symbol name individually
  for (int i = 0; i < ctx->symbols.count; i++) {
    free(ctx->symbols.names[i]);
  }
  free(ctx->symbols.names);
  free(ctx->symbols.values);
  free(ctx->symbols.types);
  free(ctx->symbols.scope_depths); // FIX

  for (int i = 0; i < ctx->structs.count; i++) {
    free(ctx->structs.names[i]);
    for (int j = 0; j < ctx->structs.field_counts[i]; j++) {
      free(ctx->structs.field_names[i][j]);
    }
    free(ctx->structs.field_names[i]);
  }
  free(ctx->structs.names);
  free(ctx->structs.types);
  free(ctx->structs.field_names);
  free(ctx->structs.field_counts);
  free(ctx);
}

static void codegen_add_symbol(CodegenContext *ctx, const char *name,
                               LLVMValueRef value, LLVMTypeRef type) {
  if (ctx->symbols.count >= ctx->symbols.capacity) {
    int new_cap = ctx->symbols.capacity == 0 ? 16 : ctx->symbols.capacity * 2;
    ctx->symbols.names =
        (char **)realloc(ctx->symbols.names, sizeof(char *) * (size_t)new_cap);
    ctx->symbols.values = (LLVMValueRef *)realloc(
        ctx->symbols.values, sizeof(LLVMValueRef) * (size_t)new_cap);
    ctx->symbols.types = (LLVMTypeRef *)realloc(
        ctx->symbols.types, sizeof(LLVMTypeRef) * (size_t)new_cap);
    ctx->symbols.scope_depths = (int *)realloc( // FIX
        ctx->symbols.scope_depths, sizeof(int) * (size_t)new_cap);
    ctx->symbols.capacity = new_cap;
  }

  ctx->symbols.names[ctx->symbols.count] = strdup(name);
  ctx->symbols.values[ctx->symbols.count] = value;
  ctx->symbols.types[ctx->symbols.count] = type;
  ctx->symbols.scope_depths[ctx->symbols.count] = ctx->current_scope; // FIX
  ctx->symbols.count++;
}

// FIX: Enter a new scope
static void codegen_push_scope(CodegenContext *ctx) { ctx->current_scope++; }

// FIX: Exit current scope, removing all symbols defined in it
static void codegen_pop_scope(CodegenContext *ctx) {
  int i = ctx->symbols.count - 1;
  while (i >= 0 && ctx->symbols.scope_depths[i] == ctx->current_scope) {
    free(ctx->symbols.names[i]);
    ctx->symbols.names[i] = NULL;
    i--;
  }
  ctx->symbols.count = i + 1;
  ctx->current_scope--;
}

static void codegen_add_struct(CodegenContext *ctx, const char *name,
                               LLVMTypeRef type, char **field_names,
                               int field_count) {
  if (ctx->structs.count >= ctx->structs.capacity) {
    int new_cap = ctx->structs.capacity == 0 ? 16 : ctx->structs.capacity * 2;
    ctx->structs.names =
        (char **)realloc(ctx->structs.names, sizeof(char *) * (size_t)new_cap);
    ctx->structs.types = (LLVMTypeRef *)realloc(
        ctx->structs.types, sizeof(LLVMTypeRef) * (size_t)new_cap);
    ctx->structs.field_names = (char ***)realloc(
        ctx->structs.field_names, sizeof(char **) * (size_t)new_cap);
    ctx->structs.field_counts = (int *)realloc(ctx->structs.field_counts,
                                               sizeof(int) * (size_t)new_cap);
    ctx->structs.capacity = new_cap;
  }
  ctx->structs.names[ctx->structs.count] = strdup(name);
  ctx->structs.types[ctx->structs.count] = type;
  ctx->structs.field_counts[ctx->structs.count] = field_count;
  ctx->structs.field_names[ctx->structs.count] =
      (char **)malloc(sizeof(char *) * (size_t)field_count);
  for (int i = 0; i < field_count; i++) {
    ctx->structs.field_names[ctx->structs.count][i] = strdup(field_names[i]);
  }
  ctx->structs.count++;
}

static int codegen_find_field(CodegenContext *ctx, const char *struct_name,
                              const char *field_name) {
  for (int i = 0; i < ctx->structs.count; i++) {
    if (strcmp(ctx->structs.names[i], struct_name) == 0) {
      for (int j = 0; j < ctx->structs.field_counts[i]; j++) {
        if (strcmp(ctx->structs.field_names[i][j], field_name) == 0) {
          return j;
        }
      }
    }
  }
  return -1;
}

static LLVMTypeRef codegen_find_struct(CodegenContext *ctx, const char *name) {
  for (int i = 0; i < ctx->structs.count; i++) {
    if (strcmp(ctx->structs.names[i], name) == 0) {
      return ctx->structs.types[i];
    }
  }
  return NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// Type Conversion (Nova -> LLVM)
// ═══════════════════════════════════════════════════════════════════════════

static LLVMTypeRef codegen_type(CodegenContext *ctx, nova_type_t *type) {
  if (!type)
    return LLVMInt64TypeInContext(ctx->llvm_ctx->context);

  switch (type->kind) {
  case TYPE_VOID:
    return LLVMVoidTypeInContext(ctx->llvm_ctx->context);
  case TYPE_BOOL:
    return LLVMInt1TypeInContext(ctx->llvm_ctx->context);
  case TYPE_I32:
  case TYPE_I64:
  case TYPE_USIZE:
    return LLVMInt64TypeInContext(ctx->llvm_ctx->context);
  case TYPE_F32:
    return LLVMFloatTypeInContext(ctx->llvm_ctx->context);
  case TYPE_F64:
    return LLVMDoubleTypeInContext(ctx->llvm_ctx->context);
  case TYPE_PTR:
  case TYPE_PTR_MUT:
  case TYPE_POINTER:
    if (type->data.ptr.pointee) {
      LLVMTypeRef pointee = codegen_type(ctx, type->data.ptr.pointee);
      return LLVMPointerType(pointee, 0);
    }
    return LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
  case TYPE_STR:
    return LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
  case TYPE_DATA: {
    LLVMTypeRef s = codegen_find_struct(ctx, type->data.name);
    if (s)
      return s;
    return LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
  }
  default:
    return LLVMInt64TypeInContext(ctx->llvm_ctx->context);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Expression Code Generation
// ═══════════════════════════════════════════════════════════════════════════

static LLVMValueRef codegen_expr(CodegenContext *ctx, nova_expr_t *expr);

static LLVMValueRef codegen_expr_literal(CodegenContext *ctx,
                                         nova_expr_t *expr) {
  switch (expr->kind) {
  case EXPR_INT:
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->llvm_ctx->context),
                        (unsigned long long)expr->data.lit_int, 1);
  case EXPR_FLOAT:
    return LLVMConstReal(LLVMDoubleTypeInContext(ctx->llvm_ctx->context),
                         expr->data.lit_float);
  case EXPR_BOOL:
    return LLVMConstInt(LLVMInt1TypeInContext(ctx->llvm_ctx->context),
                        expr->data.lit_bool ? 1 : 0, 0);
  default:
    return NULL;
  }
}

// FIX: Helper to find a symbol and return its index (searches innermost scope
// first)
static int codegen_find_symbol(CodegenContext *ctx, const char *name) {
  for (int i = ctx->symbols.count - 1; i >= 0; i--) {
    if (ctx->symbols.names[i] && strcmp(ctx->symbols.names[i], name) == 0) {
      return i;
    }
  }
  return -1;
}

static LLVMValueRef codegen_expr_ident(CodegenContext *ctx, nova_expr_t *expr) {
  int idx = codegen_find_symbol(ctx, expr->data.ident);

  if (idx == -1) {
    fprintf(stderr, "Undefined variable: %s\n", expr->data.ident);
    return NULL;
  }

  LLVMValueRef var = ctx->symbols.values[idx];
  LLVMTypeRef type = ctx->symbols.types[idx];
  return LLVMBuildLoad2(ctx->llvm_ctx->builder, type, var, "load");
}

// FIX: Returns the alloca pointer for a variable (for assignment targets)
static LLVMValueRef codegen_expr_lvalue(CodegenContext *ctx,
                                        nova_expr_t *expr) {
  if (expr->kind == EXPR_IDENT) {
    int idx = codegen_find_symbol(ctx, expr->data.ident);
    if (idx == -1) {
      fprintf(stderr, "Undefined variable: %s\n", expr->data.ident);
      return NULL;
    }
    return ctx->symbols.values[idx];
  }
  if (expr->kind == EXPR_FIELD_ACCESS) {
    LLVMValueRef obj = codegen_expr_lvalue(ctx, expr->data.field_access.object);
    if (!obj)
      return NULL;
    if (!expr->data.field_access.object->type ||
        expr->data.field_access.object->type->kind != TYPE_DATA)
      return NULL;
    const char *sname = expr->data.field_access.object->type->data.name;
    LLVMTypeRef stype = codegen_find_struct(ctx, sname);
    if (!stype)
      return NULL;
    int idx = codegen_find_field(ctx, sname, expr->data.field_access.field);
    if (idx == -1)
      return NULL;
    return LLVMBuildStructGEP2(ctx->llvm_ctx->builder, stype, obj,
                               (unsigned)idx, "lval_field_ptr");
  }
  fprintf(stderr, "Expression is not assignable\n");
  return NULL;
}

static LLVMValueRef codegen_expr_binary(CodegenContext *ctx,
                                        nova_expr_t *expr) {
  const char *op = expr->data.binary.op;

  // FIX: Handle assignment operator before evaluating both sides
  if (strcmp(op, "=") == 0) {
    LLVMValueRef lval = codegen_expr_lvalue(ctx, expr->data.binary.left);
    if (!lval)
      return NULL;
    LLVMValueRef rval = codegen_expr(ctx, expr->data.binary.right);
    if (!rval)
      return NULL;
    LLVMBuildStore(ctx->llvm_ctx->builder, rval, lval);
    return rval; // assignment expression returns the assigned value
  }

  LLVMValueRef left = codegen_expr(ctx, expr->data.binary.left);
  LLVMValueRef right = codegen_expr(ctx, expr->data.binary.right);

  if (!left || !right)
    return NULL;

  if (strcmp(op, "+") == 0) {
    return LLVMBuildAdd(ctx->llvm_ctx->builder, left, right, "add");
  } else if (strcmp(op, "-") == 0) {
    return LLVMBuildSub(ctx->llvm_ctx->builder, left, right, "sub");
  } else if (strcmp(op, "*") == 0) {
    return LLVMBuildMul(ctx->llvm_ctx->builder, left, right, "mul");
  } else if (strcmp(op, "/") == 0) {
    return LLVMBuildSDiv(ctx->llvm_ctx->builder, left, right, "div");
  } else if (strcmp(op, "%") == 0) {
    return LLVMBuildSRem(ctx->llvm_ctx->builder, left, right, "rem");
  } else if (strcmp(op, "==") == 0) {
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntEQ, left, right, "eq");
  } else if (strcmp(op, "!=") == 0) {
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntNE, left, right, "ne");
  } else if (strcmp(op, "<") == 0) {
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntSLT, left, right, "lt");
  } else if (strcmp(op, "<=") == 0) {
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntSLE, left, right, "le");
  } else if (strcmp(op, ">") == 0) {
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntSGT, left, right, "gt");
  } else if (strcmp(op, ">=") == 0) {
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntSGE, left, right, "ge");
  } else if (strcmp(op, "&&") == 0) {
    return LLVMBuildAnd(ctx->llvm_ctx->builder, left, right, "and");
  } else if (strcmp(op, "||") == 0) {
    return LLVMBuildOr(ctx->llvm_ctx->builder, left, right, "or");
  } else if (strcmp(op, "&") == 0) {
    return LLVMBuildAnd(ctx->llvm_ctx->builder, left, right, "band");
  } else if (strcmp(op, "|") == 0) {
    return LLVMBuildOr(ctx->llvm_ctx->builder, left, right, "bor");
  } else if (strcmp(op, "^") == 0) {
    return LLVMBuildXor(ctx->llvm_ctx->builder, left, right, "xor");
  } else if (strcmp(op, "<<") == 0) {
    return LLVMBuildShl(ctx->llvm_ctx->builder, left, right, "shl");
  } else if (strcmp(op, ">>") == 0) {
    return LLVMBuildAShr(ctx->llvm_ctx->builder, left, right, "shr");
  }

  fprintf(stderr, "Unsupported binary operator: %s\n", op);
  return NULL;
}

// FIX: Unary expression codegen
static LLVMValueRef codegen_expr_unary(CodegenContext *ctx, nova_expr_t *expr) {
  const char *op = expr->data.unary.op;
  LLVMValueRef operand = codegen_expr(ctx, expr->data.unary.operand);
  if (!operand)
    return NULL;

  if (strcmp(op, "-") == 0) {
    return LLVMBuildNeg(ctx->llvm_ctx->builder, operand, "neg");
  } else if (strcmp(op, "!") == 0) {
    // !x == (x == 0)
    LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(operand), 0, 0);
    return LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntEQ, operand, zero,
                         "not");
  } else if (strcmp(op, "~") == 0) {
    return LLVMBuildNot(ctx->llvm_ctx->builder, operand, "bnot");
  } else if (strcmp(op, "&") == 0) {
    // Address-of: return the alloca pointer directly
    if (expr->data.unary.operand->kind == EXPR_IDENT) {
      int idx = codegen_find_symbol(ctx, expr->data.unary.operand->data.ident);
      if (idx != -1)
        return ctx->symbols.values[idx];
    }
    fprintf(stderr, "Address-of on non-variable\n");
    return NULL;
  } else if (strcmp(op, "*") == 0) {
    // Dereference
    LLVMTypeRef pointee = LLVMInt64TypeInContext(ctx->llvm_ctx->context);
    return LLVMBuildLoad2(ctx->llvm_ctx->builder, pointee, operand, "deref");
  }

  fprintf(stderr, "Unsupported unary operator: %s\n", op);
  return NULL;
}

static LLVMValueRef codegen_declare_standard_func(CodegenContext *ctx,
                                                  const char *name) {
  LLVMTypeRef ret_type;
  LLVMTypeRef *param_types = NULL;
  unsigned param_count = 0;

  if (strcmp(name, "println") == 0 || strcmp(name, "print") == 0) {
    ret_type = LLVMVoidTypeInContext(ctx->llvm_ctx->context);
    param_types = malloc(sizeof(LLVMTypeRef));
    param_types[0] =
        LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
    param_count = 1;
  } else if (strcmp(name, "format") == 0) {
    ret_type =
        LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
    param_types = malloc(sizeof(LLVMTypeRef));
    param_types[0] =
        LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
    param_count = 1;
  } else if (strcmp(name, "mount") == 0) {
    ret_type = LLVMVoidTypeInContext(ctx->llvm_ctx->context);
    param_types = malloc(sizeof(LLVMTypeRef));
    param_types[0] =
        LLVMPointerType(LLVMInt8TypeInContext(ctx->llvm_ctx->context), 0);
    param_count = 1;
  } else if (strcmp(name, "grad") == 0) {
    ret_type = LLVMInt64TypeInContext(ctx->llvm_ctx->context);
    param_types = malloc(sizeof(LLVMTypeRef) * 2);
    param_types[0] = LLVMDoubleTypeInContext(ctx->llvm_ctx->context);
    param_types[1] = LLVMInt64TypeInContext(ctx->llvm_ctx->context);
    param_count = 2;
  } else {
    return NULL;
  }

  LLVMTypeRef ft = LLVMFunctionType(ret_type, param_types, param_count, 1);
  LLVMValueRef f = LLVMAddFunction(ctx->llvm_ctx->module, name, ft);
  free(param_types);
  return f;
}

static char *codegen_get_call_name(nova_expr_t *expr) {
  if (expr->kind == EXPR_IDENT) {
    return strdup(expr->data.ident);
  } else if (expr->kind == EXPR_NAMESPACED_ACCESS) {
    char *obj_name = codegen_get_call_name(expr->data.namespaced_access.object);
    if (!obj_name)
      return strdup(expr->data.namespaced_access.member);
    size_t len =
        strlen(obj_name) + strlen(expr->data.namespaced_access.member) + 2;
    char *res = malloc(len);
    snprintf(res, len, "%s_%s", obj_name, expr->data.namespaced_access.member);
    free(obj_name);
    return res;
  } else if (expr->kind == EXPR_FIELD_ACCESS) {
    char *obj_name = codegen_get_call_name(expr->data.field_access.object);
    if (!obj_name)
      return strdup(expr->data.field_access.field);
    size_t len = strlen(obj_name) + strlen(expr->data.field_access.field) + 2;
    char *res = malloc(len);
    snprintf(res, len, "%s_%s", obj_name, expr->data.field_access.field);
    free(obj_name);
    return res;
  }
  return NULL;
}

static LLVMValueRef codegen_get_function(CodegenContext *ctx,
                                         const char *name) {
  LLVMValueRef f = LLVMGetNamedFunction(ctx->llvm_ctx->module, name);
  if (f)
    return f;
  return codegen_declare_standard_func(ctx, name);
}

static LLVMValueRef codegen_expr_call(CodegenContext *ctx, nova_expr_t *expr) {
  char *func_name = codegen_get_call_name(expr->data.call.func);
  if (!func_name) {
    fprintf(stderr, "Only direct function calls supported (callee kind: %d)\n",
            expr->data.call.func->kind);
    return NULL;
  }

  LLVMValueRef func = codegen_get_function(ctx, func_name);
  if (!func) {
    fprintf(stderr, "Undefined function: %s\n", func_name);
    free(func_name);
    return NULL;
  }
  free(func_name);

  LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
  unsigned num_params = LLVMCountParamTypes(func_type);
  LLVMTypeRef *param_types = NULL;
  if (num_params > 0) {
    param_types = malloc(sizeof(LLVMTypeRef) * num_params);
    LLVMGetParamTypes(func_type, param_types);
  }

  LLVMValueRef *args = NULL;
  if (expr->data.call.arg_count > 0) {
    args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) *
                                  expr->data.call.arg_count);
    for (size_t i = 0; i < expr->data.call.arg_count; i++) {
      args[i] = codegen_expr(ctx, expr->data.call.args[i]);
      if (!args[i]) {
        if (param_types)
          free(param_types);
        free(args);
        return NULL;
      }

      // Auto-cast between i64 and ptr if needed
      if (i < (size_t)num_params) {
        LLVMTypeRef arg_type = LLVMTypeOf(args[i]);
        LLVMTypeRef param_type = param_types[i];
        if (LLVMGetTypeKind(arg_type) != LLVMGetTypeKind(param_type)) {
          if (LLVMGetTypeKind(arg_type) == LLVMIntegerTypeKind &&
              LLVMGetTypeKind(param_type) == LLVMPointerTypeKind) {
            args[i] = LLVMBuildIntToPtr(ctx->llvm_ctx->builder, args[i],
                                        param_type, "ptr_cast");
          } else if (LLVMGetTypeKind(arg_type) == LLVMPointerTypeKind &&
                     LLVMGetTypeKind(param_type) == LLVMIntegerTypeKind) {
            args[i] = LLVMBuildPtrToInt(
                ctx->llvm_ctx->builder, args[i],
                LLVMInt64TypeInContext(ctx->llvm_ctx->context), "int_cast");
          }
        }
      }
    }
  }

  LLVMTypeRef ret_type = LLVMGetReturnType(func_type);
  const char *name =
      (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) ? "" : "call";

  LLVMValueRef result =
      LLVMBuildCall2(ctx->llvm_ctx->builder, func_type, func, args,
                     (unsigned)expr->data.call.arg_count, name);

  if (param_types)
    free(param_types);
  free(args);
  return result;
}

static LLVMValueRef codegen_expr_field_access(CodegenContext *ctx,
                                              nova_expr_t *expr) {
  LLVMValueRef obj = codegen_expr(ctx, expr->data.field_access.object);
  if (!obj)
    return NULL;

  LLVMTypeRef obj_ptr_type = LLVMTypeOf(obj);
  LLVMTypeRef obj_type = NULL;

  if (LLVMGetTypeKind(obj_ptr_type) == LLVMPointerTypeKind) {
    if (expr->data.field_access.object->type &&
        expr->data.field_access.object->type->kind == TYPE_DATA) {
      obj_type = codegen_find_struct(
          ctx, expr->data.field_access.object->type->data.name);
    }
  }

  if (!obj_type || LLVMGetTypeKind(obj_type) != LLVMStructTypeKind) {
    fprintf(stderr, "Field access on non-struct type\n");
    return NULL;
  }

  int field_idx =
      codegen_find_field(ctx, expr->data.field_access.object->type->data.name,
                         expr->data.field_access.field);
  if (field_idx == -1) {
    fprintf(stderr, "Field not found: %s\n", expr->data.field_access.field);
    return NULL;
  }

  LLVMValueRef ptr = LLVMBuildStructGEP2(ctx->llvm_ctx->builder, obj_type, obj,
                                         (unsigned)field_idx, "field_ptr");
  LLVMTypeRef field_type =
      LLVMStructGetTypeAtIndex(obj_type, (unsigned)field_idx);
  return LLVMBuildLoad2(ctx->llvm_ctx->builder, field_type, ptr, "field_load");
}

static LLVMValueRef codegen_expr_struct_init(CodegenContext *ctx,
                                             nova_expr_t *expr) {
  LLVMTypeRef struct_type =
      codegen_find_struct(ctx, expr->data.struct_init.struct_name);
  if (!struct_type) {
    fprintf(stderr, "Unknown struct: %s\n", expr->data.struct_init.struct_name);
    return NULL;
  }

  LLVMValueRef alloca =
      LLVMBuildAlloca(ctx->llvm_ctx->builder, struct_type, "struct_init");

  for (size_t i = 0; i < expr->data.struct_init.field_count; i++) {
    int idx = codegen_find_field(ctx, expr->data.struct_init.struct_name,
                                 expr->data.struct_init.fields[i].name);
    if (idx != -1) {
      LLVMValueRef val =
          codegen_expr(ctx, expr->data.struct_init.fields[i].value);
      if (val) {
        LLVMValueRef field_ptr =
            LLVMBuildStructGEP2(ctx->llvm_ctx->builder, struct_type, alloca,
                                (unsigned)idx, "init_field_ptr");
        LLVMBuildStore(ctx->llvm_ctx->builder, val, field_ptr);
      }
    }
  }

  return alloca;
}

static LLVMValueRef codegen_expr(CodegenContext *ctx, nova_expr_t *expr) {
  if (!expr)
    return NULL;

  switch (expr->kind) {
  case EXPR_INT:
  case EXPR_FLOAT:
  case EXPR_BOOL:
    return codegen_expr_literal(ctx, expr);
  case EXPR_IDENT:
    return codegen_expr_ident(ctx, expr);
  case EXPR_BINARY:
    return codegen_expr_binary(ctx, expr);
  case EXPR_UNARY:
    return codegen_expr_unary(ctx, expr);
  case EXPR_CALL:
    return codegen_expr_call(ctx, expr);
  case EXPR_FIELD_ACCESS:
    return codegen_expr_field_access(ctx, expr);
  case EXPR_STRUCT_INIT:
    return codegen_expr_struct_init(ctx, expr);
  case EXPR_STR: {
    const char *str = expr->data.lit_str;
    if (!str)
      str = "";
    return LLVMBuildGlobalStringPtr(ctx->llvm_ctx->builder, str, "str");
  }
  case EXPR_NAMESPACED_ACCESS:
    return NULL;
  default:
    fprintf(stderr, "Unsupported expression kind: %d\n", expr->kind);
    return NULL;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Statement Code Generation
// ═══════════════════════════════════════════════════════════════════════════

static bool codegen_stmt(CodegenContext *ctx, nova_stmt_t *stmt);

static bool codegen_stmt_return(CodegenContext *ctx, nova_stmt_t *stmt) {
  if (stmt->data.return_expr) {
    LLVMValueRef ret_val = codegen_expr(ctx, stmt->data.return_expr);
    if (!ret_val)
      return false;
    LLVMBuildRet(ctx->llvm_ctx->builder, ret_val);
  } else {
    LLVMBuildRetVoid(ctx->llvm_ctx->builder);
  }
  return true;
}

static bool codegen_stmt_var_decl(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMTypeRef var_type = codegen_type(ctx, stmt->data.var_decl.type);
  LLVMValueRef alloca = LLVMBuildAlloca(ctx->llvm_ctx->builder, var_type,
                                        stmt->data.var_decl.name);

  if (stmt->data.var_decl.init) {
    LLVMValueRef init_val = codegen_expr(ctx, stmt->data.var_decl.init);
    if (!init_val)
      return false;
    LLVMBuildStore(ctx->llvm_ctx->builder, init_val, alloca);
  }

  codegen_add_symbol(ctx, stmt->data.var_decl.name, alloca, var_type);
  return true;
}

static bool codegen_stmt_expr(CodegenContext *ctx, nova_stmt_t *stmt) {
  codegen_expr(ctx, stmt->data.expr);
  return true;
}

static bool codegen_stmt_block(CodegenContext *ctx, nova_stmt_t *stmt) {
  codegen_push_scope(ctx);
  for (size_t i = 0; i < stmt->data.block.count; i++) {
    if (!codegen_stmt(ctx, stmt->data.block.statements[i])) {
      codegen_pop_scope(ctx);
      return false;
    }
    if (LLVMGetBasicBlockTerminator(
            LLVMGetInsertBlock(ctx->llvm_ctx->builder))) {
      break;
    }
  }
  codegen_pop_scope(ctx);
  return true;
}

static bool codegen_stmt_struct_decl(CodegenContext *ctx, nova_stmt_t *stmt) {
  nova_struct_decl_t *s = stmt->data.struct_decl;
  LLVMTypeRef *fields = malloc(sizeof(LLVMTypeRef) * s->field_count);
  char **names = malloc(sizeof(char *) * s->field_count);
  for (size_t i = 0; i < s->field_count; i++) {
    fields[i] = codegen_type(ctx, s->fields[i].type);
    names[i] = s->fields[i].name;
  }
  LLVMTypeRef struct_type =
      LLVMStructCreateNamed(ctx->llvm_ctx->context, s->name);
  LLVMStructSetBody(struct_type, fields, (unsigned)s->field_count, 0);
  codegen_add_struct(ctx, s->name, struct_type, names, (int)s->field_count);
  free(fields);
  free(names);
  return true;
}

static bool codegen_stmt_check(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMValueRef cond = codegen_expr(ctx, stmt->data.check_stmt.condition);
  if (!cond)
    return false;

  cond = LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntNE, cond,
                       LLVMConstInt(LLVMTypeOf(cond), 0, 0), "ifcond");

  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(
      ctx->llvm_ctx->context, ctx->current_function, "then");
  LLVMBasicBlockRef else_bb = LLVMAppendBasicBlockInContext(
      ctx->llvm_ctx->context, ctx->current_function, "else");
  LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlockInContext(
      ctx->llvm_ctx->context, ctx->current_function, "ifcont");

  LLVMBuildCondBr(ctx->llvm_ctx->builder, cond, then_bb, else_bb);

  // Then block
  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, then_bb);
  codegen_push_scope(ctx);
  if (!codegen_stmt(ctx, stmt->data.check_stmt.then_branch)) {
    codegen_pop_scope(ctx);
    return false;
  }
  codegen_pop_scope(ctx);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->llvm_ctx->builder)))
    LLVMBuildBr(ctx->llvm_ctx->builder, merge_bb);

  // Else block
  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, else_bb);
  codegen_push_scope(ctx);
  if (stmt->data.check_stmt.else_branch) {
    if (!codegen_stmt(ctx, stmt->data.check_stmt.else_branch)) {
      codegen_pop_scope(ctx);
      return false;
    }
  }
  codegen_pop_scope(ctx);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->llvm_ctx->builder)))
    LLVMBuildBr(ctx->llvm_ctx->builder, merge_bb);

  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, merge_bb);
  return true;
}

static bool codegen_stmt_while(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlockInContext(
      ctx->llvm_ctx->context, ctx->current_function, "whilecond");
  LLVMBasicBlockRef body_bb = LLVMAppendBasicBlockInContext(
      ctx->llvm_ctx->context, ctx->current_function, "whilebody");
  LLVMBasicBlockRef end_bb = LLVMAppendBasicBlockInContext(
      ctx->llvm_ctx->context, ctx->current_function, "whileend");

  LLVMBuildBr(ctx->llvm_ctx->builder, cond_bb);

  // Cond block
  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, cond_bb);
  LLVMValueRef cond = codegen_expr(ctx, stmt->data.while_stmt.condition);
  if (!cond)
    return false;
  cond = LLVMBuildICmp(ctx->llvm_ctx->builder, LLVMIntNE, cond,
                       LLVMConstInt(LLVMTypeOf(cond), 0, 0), "while_cond_v");
  LLVMBuildCondBr(ctx->llvm_ctx->builder, cond, body_bb, end_bb);

  // Body block
  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, body_bb);
  codegen_push_scope(ctx);
  if (!codegen_stmt(ctx, stmt->data.while_stmt.body)) {
    codegen_pop_scope(ctx);
    return false;
  }
  codegen_pop_scope(ctx);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->llvm_ctx->builder)))
    LLVMBuildBr(ctx->llvm_ctx->builder, cond_bb);

  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, end_bb);
  return true;
}

static bool codegen_stmt(CodegenContext *ctx, nova_stmt_t *stmt) {
  if (!stmt)
    return true;

  switch (stmt->kind) {
  case STMT_RETURN:
    return codegen_stmt_return(ctx, stmt);
  case STMT_VAR_DECL:
    return codegen_stmt_var_decl(ctx, stmt);
  case STMT_EXPR:
    return codegen_stmt_expr(ctx, stmt);
  case STMT_BLOCK:
    return codegen_stmt_block(ctx, stmt);
  case STMT_STRUCT_DECL:
    return codegen_stmt_struct_decl(ctx, stmt);
  case STMT_CHECK:
    return codegen_stmt_check(ctx, stmt);
  case STMT_WHILE:
    return codegen_stmt_while(ctx, stmt);
  case STMT_IMPORT:
  case STMT_ENUM_DECL:
    return true;
  default:
    fprintf(stderr, "Unsupported statement kind: %d\n", stmt->kind);
    return false;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Function Code Generation
// ═══════════════════════════════════════════════════════════════════════════

static bool codegen_function_prototype(CodegenContext *ctx,
                                       nova_stmt_t *fn_stmt) {
  if (fn_stmt->kind != STMT_FN)
    return false;

  LLVMTypeRef *param_types = NULL;
  if (fn_stmt->data.fn_stmt.param_count > 0) {
    param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) *
                                        fn_stmt->data.fn_stmt.param_count);
    for (size_t i = 0; i < fn_stmt->data.fn_stmt.param_count; i++) {
      param_types[i] = codegen_type(ctx, fn_stmt->data.fn_stmt.params[i]->type);
    }
  }

  LLVMTypeRef ret_type = codegen_type(ctx, fn_stmt->data.fn_stmt.return_type);
  LLVMTypeRef func_type = LLVMFunctionType(
      ret_type, param_types, (unsigned)fn_stmt->data.fn_stmt.param_count, 0);

  LLVMAddFunction(ctx->llvm_ctx->module, fn_stmt->data.fn_stmt.name, func_type);

  if (param_types)
    free(param_types);
  return true;
}

static bool codegen_function(CodegenContext *ctx, nova_stmt_t *fn_stmt) {
  if (fn_stmt->kind != STMT_FN)
    return false;

  LLVMValueRef func =
      LLVMGetNamedFunction(ctx->llvm_ctx->module, fn_stmt->data.fn_stmt.name);
  if (!func) {
    if (!codegen_function_prototype(ctx, fn_stmt))
      return false;
    func =
        LLVMGetNamedFunction(ctx->llvm_ctx->module, fn_stmt->data.fn_stmt.name);
  }

  ctx->current_function = func;

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(ctx->llvm_ctx->context, func, "entry");
  LLVMPositionBuilderAtEnd(ctx->llvm_ctx->builder, entry);

  LLVMTypeRef *param_types = NULL;
  if (fn_stmt->data.fn_stmt.param_count > 0) {
    param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) *
                                        fn_stmt->data.fn_stmt.param_count);
    for (size_t i = 0; i < fn_stmt->data.fn_stmt.param_count; i++) {
      param_types[i] = codegen_type(ctx, fn_stmt->data.fn_stmt.params[i]->type);
    }
  }
  LLVMTypeRef ret_type = codegen_type(ctx, fn_stmt->data.fn_stmt.return_type);

  codegen_push_scope(ctx);

  for (size_t i = 0; i < fn_stmt->data.fn_stmt.param_count; i++) {
    LLVMValueRef param = LLVMGetParam(func, (unsigned)i);
    LLVMSetValueName2(param, fn_stmt->data.fn_stmt.params[i]->name,
                      strlen(fn_stmt->data.fn_stmt.params[i]->name));

    LLVMValueRef alloca =
        LLVMBuildAlloca(ctx->llvm_ctx->builder, param_types[i],
                        fn_stmt->data.fn_stmt.params[i]->name);
    LLVMBuildStore(ctx->llvm_ctx->builder, param, alloca);
    codegen_add_symbol(ctx, fn_stmt->data.fn_stmt.params[i]->name, alloca,
                       param_types[i]);
  }

  bool success = codegen_stmt(ctx, fn_stmt->data.fn_stmt.body);

  // Ensure terminator
  if (success) {
    LLVMBasicBlockRef last_bb = LLVMGetInsertBlock(ctx->llvm_ctx->builder);
    if (!LLVMGetBasicBlockTerminator(last_bb)) {
      if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) {
        LLVMBuildRetVoid(ctx->llvm_ctx->builder);
      } else {
        LLVMBuildRet(ctx->llvm_ctx->builder, LLVMConstNull(ret_type));
      }
    }
  }

  codegen_pop_scope(ctx);

  if (param_types)
    free(param_types);
  return success;
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════

static void codegen_visit_functions_prototype(CodegenContext *ctx,
                                              nova_stmt_t *stmt) {
  if (!stmt)
    return;
  if (stmt->kind == STMT_FN) {
    codegen_function_prototype(ctx, stmt);
  } else if (stmt->kind == STMT_BLOCK) {
    for (size_t i = 0; i < stmt->data.block.count; i++) {
      codegen_visit_functions_prototype(ctx, stmt->data.block.statements[i]);
    }
  }
}

static bool codegen_visit_functions_body(CodegenContext *ctx,
                                         nova_stmt_t *stmt) {
  if (!stmt)
    return true;
  if (stmt->kind == STMT_FN) {
    return codegen_function(ctx, stmt);
  } else if (stmt->kind == STMT_BLOCK) {
    for (size_t i = 0; i < stmt->data.block.count; i++) {
      if (!codegen_visit_functions_body(ctx, stmt->data.block.statements[i]))
        return false;
    }
  }
  return true;
}

bool nova_llvm_codegen_program(NovaLLVMContext *llvm_ctx,
                               nova_program_t *program) {
  if (!llvm_ctx || !program)
    return false;

  fprintf(stderr, "SUPREME LLVM CODEGEN STARTING (Stage 0 v10 Support)...\n");

  CodegenContext *ctx = codegen_context_create(llvm_ctx);
  bool success = true;

  // Pass 1: Generate prototypes
  for (size_t i = 0; i < program->declaration_count; i++) {
    nova_top_level_t *decl = program->declarations[i];
    if (!decl)
      continue;
    if (decl->kind == DECL_FUNCTION) {
      codegen_function_prototype(ctx, (nova_stmt_t *)decl->data);
    } else if (decl->kind == DECL_VAR) {
      codegen_visit_functions_prototype(ctx, (nova_stmt_t *)decl->data);
    }
  }

  // Pass 2: Generate bodies
  for (size_t i = 0; i < program->declaration_count; i++) {
    nova_top_level_t *decl = program->declarations[i];
    if (!decl)
      continue;

    if (decl->kind == DECL_FUNCTION) {
      if (!codegen_function(ctx, (nova_stmt_t *)decl->data)) {
        success = false;
        break;
      }
    } else if (decl->kind == DECL_VAR) {
      if (!codegen_visit_functions_body(ctx, (nova_stmt_t *)decl->data)) {
        success = false;
        break;
      }
    }
  }

  codegen_context_destroy(ctx);
  return success;
}

#else // !HAVE_LLVM

bool nova_llvm_codegen_program(NovaLLVMContext *llvm_ctx,
                               nova_program_t *program) {
  (void)llvm_ctx;
  (void)program;
  fprintf(stderr, "LLVM support not compiled in\n");
  return false;
}

#endif // HAVE_LLVM
