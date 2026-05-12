/**
 * Nova LLVM IR Code Generator
 *
 * Converts Nova AST to LLVM IR for JIT/AOT compilation.
 * Handles: functions, variables, if/else, while, for, expressions, AD grad().
 */

#include "ast.h"
#include "nova_llvm_ad.h"
#include "nova_llvm_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>

// ═══════════════════════════════════════════════════════════════════════════
// Symbol Table
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char         *name;
  LLVMValueRef  alloca;
  LLVMTypeRef   type;   /* element type — used for correct Load2 */
} SymbolEntry;

typedef struct {
  NovaLLVMContext *llvm_ctx;
  LLVMContextRef   lctx;
  LLVMBuilderRef   builder;
  LLVMModuleRef    module;
  LLVMValueRef     current_function;
  LLVMTypeRef      current_ret_type; /* for implicit return synthesis */
  struct {
    SymbolEntry *entries;
    int          count;
    int          capacity;
  } symbols;
} CodegenContext;

static CodegenContext *codegen_context_create(NovaLLVMContext *llvm_ctx) {
  CodegenContext *ctx = (CodegenContext *)calloc(1, sizeof(CodegenContext));
  if (!ctx) return NULL;
  ctx->llvm_ctx = llvm_ctx;
  ctx->lctx     = nova_llvm_get_llvm_context(llvm_ctx);
  ctx->builder  = nova_llvm_get_builder(llvm_ctx);
  ctx->module   = nova_llvm_get_module(llvm_ctx);
  return ctx;
}

static void codegen_context_destroy(CodegenContext *ctx) {
  if (!ctx) return;
  for (int i = 0; i < ctx->symbols.count; i++)
    free(ctx->symbols.entries[i].name);
  free(ctx->symbols.entries);
  free(ctx);
}

static void codegen_add_symbol(CodegenContext *ctx, const char *name,
                               LLVMValueRef alloca, LLVMTypeRef type) {
  if (ctx->symbols.count >= ctx->symbols.capacity) {
    int nc = ctx->symbols.capacity == 0 ? 16 : ctx->symbols.capacity * 2;
    ctx->symbols.entries = (SymbolEntry *)realloc(
        ctx->symbols.entries, sizeof(SymbolEntry) * (size_t)nc);
    if (!ctx->symbols.entries) return;
    ctx->symbols.capacity = nc;
  }
  SymbolEntry *e = &ctx->symbols.entries[ctx->symbols.count++];
  e->name   = strdup(name);
  e->alloca = alloca;
  e->type   = type;
}

/* Returns the SymbolEntry or NULL.  Reverse scan: inner scope wins. */
static SymbolEntry *codegen_find_symbol(CodegenContext *ctx, const char *name) {
  for (int i = ctx->symbols.count - 1; i >= 0; i--)
    if (strcmp(ctx->symbols.entries[i].name, name) == 0)
      return &ctx->symbols.entries[i];
  return NULL;
}

/* Save/restore scope boundary for block statements */
static int codegen_scope_save(CodegenContext *ctx) {
  return ctx->symbols.count;
}
static void codegen_scope_restore(CodegenContext *ctx, int saved) {
  for (int i = saved; i < ctx->symbols.count; i++)
    free(ctx->symbols.entries[i].name);
  ctx->symbols.count = saved;
}

// ═══════════════════════════════════════════════════════════════════════════
// Type Conversion  Nova → LLVM
// ═══════════════════════════════════════════════════════════════════════════

static LLVMTypeRef codegen_type(CodegenContext *ctx, nova_type_t *type) {
  if (!type) return LLVMInt64TypeInContext(ctx->lctx);

  switch (type->kind) {
  case TYPE_VOID:    return LLVMVoidTypeInContext(ctx->lctx);
  case TYPE_BOOL:    return LLVMInt1TypeInContext(ctx->lctx);
  case TYPE_I32:     return LLVMInt32TypeInContext(ctx->lctx);   /* was wrongly i64 */
  case TYPE_I64:
  case TYPE_USIZE:   return LLVMInt64TypeInContext(ctx->lctx);
  case TYPE_U8:      return LLVMInt8TypeInContext(ctx->lctx);
  case TYPE_F32:     return LLVMFloatTypeInContext(ctx->lctx);
  case TYPE_F64:     return LLVMDoubleTypeInContext(ctx->lctx);
  case TYPE_STR:
    /* *i8 — C-string representation */
    return LLVMPointerType(LLVMInt8TypeInContext(ctx->lctx), 0);
  case TYPE_PTR:
  case TYPE_PTR_MUT:
  case TYPE_POINTER:
    if (type->data.ptr.pointee) {
      LLVMTypeRef pointee = codegen_type(ctx, type->data.ptr.pointee);
      return LLVMPointerType(pointee, 0);
    }
    return LLVMPointerType(LLVMInt8TypeInContext(ctx->lctx), 0);
  case TYPE_ARRAY: {
    LLVMTypeRef elem = codegen_type(ctx, type->data.ptr.pointee);
    return LLVMArrayType(elem, (unsigned)type->data.ptr.size);
  }
  default:
    return LLVMInt64TypeInContext(ctx->lctx); /* safe fallback */
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Forward declarations
// ═══════════════════════════════════════════════════════════════════════════

static LLVMValueRef codegen_expr(CodegenContext *ctx, nova_expr_t *expr);
static bool         codegen_stmt(CodegenContext *ctx, nova_stmt_t *stmt);

// ═══════════════════════════════════════════════════════════════════════════
// Expression Code Generation
// ═══════════════════════════════════════════════════════════════════════════

static LLVMValueRef codegen_expr_literal(CodegenContext *ctx, nova_expr_t *expr) {
  switch (expr->kind) {
  case EXPR_INT:
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->lctx),
                        (unsigned long long)expr->data.lit_int, /*sign_extend=*/1);
  case EXPR_FLOAT:
    return LLVMConstReal(LLVMDoubleTypeInContext(ctx->lctx), expr->data.lit_float);
  case EXPR_BOOL:
    return LLVMConstInt(LLVMInt1TypeInContext(ctx->lctx),
                        expr->data.lit_bool ? 1 : 0, 0);
  default: return NULL;
  }
}

static LLVMValueRef codegen_expr_ident(CodegenContext *ctx, nova_expr_t *expr) {
  SymbolEntry *e = codegen_find_symbol(ctx, expr->data.ident);
  if (!e) {
    fprintf(stderr, "codegen: undefined variable '%s'\n", expr->data.ident);
    return NULL;
  }
  return LLVMBuildLoad2(ctx->builder, e->type, e->alloca, "load");
}

/* EXPR_ASSIGN  name = value  (must already be declared) */
static LLVMValueRef codegen_expr_assign(CodegenContext *ctx, nova_expr_t *expr) {
  SymbolEntry *e = codegen_find_symbol(ctx, expr->data.assign.name);
  if (!e) {
    fprintf(stderr, "codegen: assignment to undeclared variable '%s'\n",
            expr->data.assign.name);
    return NULL;
  }
  LLVMValueRef val = codegen_expr(ctx, expr->data.assign.value);
  if (!val) return NULL;

  /* Widen integer if needed (e.g. bool assigned to i64 slot) */
  LLVMTypeRef val_t = LLVMTypeOf(val);
  if (val_t != e->type &&
      LLVMGetTypeKind(val_t)  == LLVMIntegerTypeKind &&
      LLVMGetTypeKind(e->type) == LLVMIntegerTypeKind) {
    val = LLVMBuildSExt(ctx->builder, val, e->type, "sext");
  }
  LLVMBuildStore(ctx->builder, val, e->alloca);
  return val;
}

static LLVMValueRef codegen_expr_binary(CodegenContext *ctx, nova_expr_t *expr) {
  /* Short-circuit logical operators */
  const char *op = expr->data.binary.op;
  if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
    LLVMValueRef lhs = codegen_expr(ctx, expr->data.binary.left);
    if (!lhs) return NULL;
    /* Convert to i1 if needed */
    if (LLVMGetTypeKind(LLVMTypeOf(lhs)) != LLVMInt1TypeKind)
      lhs = LLVMBuildICmp(ctx->builder, LLVMIntNE, lhs,
                          LLVMConstNull(LLVMTypeOf(lhs)), "tobool");
    LLVMValueRef rhs = codegen_expr(ctx, expr->data.binary.right);
    if (!rhs) return NULL;
    if (LLVMGetTypeKind(LLVMTypeOf(rhs)) != LLVMInt1TypeKind)
      rhs = LLVMBuildICmp(ctx->builder, LLVMIntNE, rhs,
                          LLVMConstNull(LLVMTypeOf(rhs)), "tobool");
    if (strcmp(op, "&&") == 0) return LLVMBuildAnd(ctx->builder, lhs, rhs, "and");
    else                        return LLVMBuildOr (ctx->builder, lhs, rhs, "or");
  }

  LLVMValueRef left  = codegen_expr(ctx, expr->data.binary.left);
  LLVMValueRef right = codegen_expr(ctx, expr->data.binary.right);
  if (!left || !right) return NULL;

  bool is_float = (LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMFloatTypeKind ||
                   LLVMGetTypeKind(LLVMTypeOf(left)) == LLVMDoubleTypeKind);

  if      (strcmp(op, "+")  == 0)
    return is_float ? LLVMBuildFAdd(ctx->builder, left, right, "fadd")
                    : LLVMBuildAdd (ctx->builder, left, right, "add");
  else if (strcmp(op, "-")  == 0)
    return is_float ? LLVMBuildFSub(ctx->builder, left, right, "fsub")
                    : LLVMBuildSub (ctx->builder, left, right, "sub");
  else if (strcmp(op, "*")  == 0)
    return is_float ? LLVMBuildFMul(ctx->builder, left, right, "fmul")
                    : LLVMBuildMul (ctx->builder, left, right, "mul");
  else if (strcmp(op, "/")  == 0)
    return is_float ? LLVMBuildFDiv(ctx->builder, left, right, "fdiv")
                    : LLVMBuildSDiv(ctx->builder, left, right, "div");
  else if (strcmp(op, "%")  == 0)
    return LLVMBuildSRem(ctx->builder, left, right, "rem");
  else if (strcmp(op, "==") == 0)
    return is_float ? LLVMBuildFCmp(ctx->builder, LLVMRealOEQ, left, right, "feq")
                    : LLVMBuildICmp(ctx->builder, LLVMIntEQ,   left, right, "eq");
  else if (strcmp(op, "!=") == 0)
    return is_float ? LLVMBuildFCmp(ctx->builder, LLVMRealONE, left, right, "fne")
                    : LLVMBuildICmp(ctx->builder, LLVMIntNE,   left, right, "ne");
  else if (strcmp(op, "<")  == 0)
    return is_float ? LLVMBuildFCmp(ctx->builder, LLVMRealOLT, left, right, "flt")
                    : LLVMBuildICmp(ctx->builder, LLVMIntSLT,  left, right, "lt");
  else if (strcmp(op, "<=") == 0)
    return is_float ? LLVMBuildFCmp(ctx->builder, LLVMRealOLE, left, right, "fle")
                    : LLVMBuildICmp(ctx->builder, LLVMIntSLE,  left, right, "le");
  else if (strcmp(op, ">")  == 0)
    return is_float ? LLVMBuildFCmp(ctx->builder, LLVMRealOGT, left, right, "fgt")
                    : LLVMBuildICmp(ctx->builder, LLVMIntSGT,  left, right, "gt");
  else if (strcmp(op, ">=") == 0)
    return is_float ? LLVMBuildFCmp(ctx->builder, LLVMRealOGE, left, right, "fge")
                    : LLVMBuildICmp(ctx->builder, LLVMIntSGE,  left, right, "ge");

  fprintf(stderr, "codegen: unknown binary op '%s'\n", op);
  return NULL;
}

/* EXPR_CAST — numeric casts only for now */
static LLVMValueRef codegen_expr_cast(CodegenContext *ctx, nova_expr_t *expr) {
  LLVMValueRef val = codegen_expr(ctx, expr->data.cast.expr);
  if (!val) return NULL;
  LLVMTypeRef dst = codegen_type(ctx, expr->data.cast.target_type);
  LLVMTypeRef src = LLVMTypeOf(val);
  if (src == dst) return val;

  LLVMTypeKind sk = LLVMGetTypeKind(src);
  LLVMTypeKind dk = LLVMGetTypeKind(dst);

  if (sk == LLVMIntegerTypeKind && dk == LLVMIntegerTypeKind)
    return LLVMBuildIntCast2(ctx->builder, val, dst, /*signed=*/1, "icast");
  if ((sk == LLVMFloatTypeKind || sk == LLVMDoubleTypeKind) &&
      (dk == LLVMFloatTypeKind || dk == LLVMDoubleTypeKind))
    return LLVMBuildFPCast(ctx->builder, val, dst, "fpcast");
  if (sk == LLVMIntegerTypeKind &&
      (dk == LLVMFloatTypeKind || dk == LLVMDoubleTypeKind))
    return LLVMBuildSIToFP(ctx->builder, val, dst, "sitofp");
  if ((sk == LLVMFloatTypeKind || sk == LLVMDoubleTypeKind) &&
      dk == LLVMIntegerTypeKind)
    return LLVMBuildFPToSI(ctx->builder, val, dst, "fptosi");
  if (sk == LLVMIntegerTypeKind && dk == LLVMPointerTypeKind)
    return LLVMBuildIntToPtr(ctx->builder, val, dst, "inttoptr");
  if (sk == LLVMPointerTypeKind && dk == LLVMIntegerTypeKind)
    return LLVMBuildPtrToInt(ctx->builder, val, dst, "ptrtoint");

  fprintf(stderr, "codegen: unsupported cast\n");
  return val; /* best-effort: return unchanged */
}

/* EXPR_FIELD_ACCESS — struct field by index via GEP */
static LLVMValueRef codegen_expr_field_access(CodegenContext *ctx,
                                              nova_expr_t *expr) {
  /* For now: look up object in symbol table, use field name as index 0 fallback.
   * Full struct field indexing requires a type registry — emit a warning and
   * fall back to index 0 until the struct registry is wired in. */
  SymbolEntry *e = NULL;
  if (expr->data.field_access.object->kind == EXPR_IDENT)
    e = codegen_find_symbol(ctx, expr->data.field_access.object->data.ident);

  if (!e) {
    fprintf(stderr, "codegen: field access on unknown object '%s.%s'\n",
            expr->data.field_access.object->kind == EXPR_IDENT
                ? expr->data.field_access.object->data.ident : "?",
            expr->data.field_access.field);
    return NULL;
  }

  /* GEP into struct at index 0 (placeholder — real index needs type registry) */
  LLVMValueRef indices[2] = {
    LLVMConstInt(LLVMInt32TypeInContext(ctx->lctx), 0, 0),
    LLVMConstInt(LLVMInt32TypeInContext(ctx->lctx), 0, 0),
  };
  LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, e->type,
                                    e->alloca, indices, 2, "field");
  return LLVMBuildLoad2(ctx->builder, LLVMInt64TypeInContext(ctx->lctx),
                        gep, "fload");
}

/* ── Standard function declarations (runtime builtins) ─────────────────── */
static LLVMValueRef codegen_declare_standard_func(CodegenContext *ctx,
                                                  const char *name) {
  LLVMTypeRef  ret_type    = NULL;
  LLVMTypeRef *param_types = NULL;
  unsigned     param_count = 0;
  int          variadic    = 1;

  if (strcmp(name, "println") == 0 || strcmp(name, "print") == 0) {
    ret_type      = LLVMVoidTypeInContext(ctx->lctx);
    param_types   = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef));
    if (!param_types) return NULL;
    param_types[0] = LLVMPointerType(LLVMInt8TypeInContext(ctx->lctx), 0);
    param_count   = 1;
  } else if (strcmp(name, "format") == 0) {
    ret_type      = LLVMPointerType(LLVMInt8TypeInContext(ctx->lctx), 0);
    param_types   = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef));
    if (!param_types) return NULL;
    param_types[0] = LLVMPointerType(LLVMInt8TypeInContext(ctx->lctx), 0);
    param_count   = 1;
  } else if (strcmp(name, "mount") == 0) {
    ret_type      = LLVMVoidTypeInContext(ctx->lctx);
    param_types   = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef));
    if (!param_types) return NULL;
    param_types[0] = LLVMPointerType(LLVMInt8TypeInContext(ctx->lctx), 0);
    param_count   = 1;
  } else {
    /* grad() and other differentiable builtins handled before this path */
    return NULL;
  }

  LLVMTypeRef  ft = LLVMFunctionType(ret_type, param_types, param_count, variadic);
  LLVMValueRef f  = LLVMAddFunction(ctx->module, name, ft);
  free(param_types);
  return f;
}

static LLVMValueRef codegen_get_function(CodegenContext *ctx, const char *name) {
  LLVMValueRef f = LLVMGetNamedFunction(ctx->module, name);
  if (f) return f;
  return codegen_declare_standard_func(ctx, name);
}

/* ── EXPR_CALL ──────────────────────────────────────────────────────────── */
static LLVMValueRef codegen_expr_call(CodegenContext *ctx, nova_expr_t *expr) {
  if (expr->data.call.func->kind != EXPR_IDENT) {
    fprintf(stderr, "codegen: only direct function calls supported\n");
    return NULL;
  }

  const char *func_name = expr->data.call.func->data.ident;

  /* ── grad(f, args...) — IR-level AD dispatch ── */
  if (strcmp(func_name, "grad") == 0) {
    if (expr->data.call.arg_count < 1) {
      fprintf(stderr, "grad(): requires at least one argument\n");
      return NULL;
    }
    nova_expr_t *fn_arg = expr->data.call.args[0];
    const char  *target = NULL;
    if      (fn_arg->kind == EXPR_STR)   target = fn_arg->data.lit_str;
    else if (fn_arg->kind == EXPR_IDENT) target = fn_arg->data.ident;

    if (!target) {
      fprintf(stderr, "grad(): first arg must be a function name\n");
      return NULL;
    }

    int           nargs = (int)expr->data.call.arg_count - 1;
    LLVMValueRef *args  = NULL;
    if (nargs > 0) {
      args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * (size_t)nargs);
      if (!args) return NULL;
      for (int i = 0; i < nargs; i++) {
        args[i] = codegen_expr(ctx, expr->data.call.args[i + 1]);
        if (!args[i]) { free(args); return NULL; }
      }
    }
    LLVMValueRef result =
        nova_llvm_ad_grad_call(ctx->llvm_ctx, target, args, nargs);
    free(args);
    return result;
  }

  /* ── Normal call ── */
  LLVMValueRef func = codegen_get_function(ctx, func_name);
  if (!func) {
    fprintf(stderr, "codegen: undefined function '%s'\n", func_name);
    return NULL;
  }

  LLVMTypeRef  func_type  = LLVMGlobalGetValueType(func);
  unsigned     num_params = LLVMCountParamTypes(func_type);

  LLVMTypeRef *param_types = NULL;
  if (num_params > 0) {
    param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * num_params);
    if (!param_types) return NULL;
    LLVMGetParamTypes(func_type, param_types);
  }

  size_t        nargs = expr->data.call.arg_count;
  LLVMValueRef *args  = NULL;
  if (nargs > 0) {
    args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * nargs);
    if (!args) { free(param_types); return NULL; }
  }

  for (size_t i = 0; i < nargs; i++) {
    args[i] = codegen_expr(ctx, expr->data.call.args[i]);
    if (!args[i]) { free(param_types); free(args); return NULL; }

    /* Auto-cast int↔ptr if needed */
    if (i < (size_t)num_params) {
      LLVMTypeRef at = LLVMTypeOf(args[i]);
      LLVMTypeRef pt = param_types[i];
      if (LLVMGetTypeKind(at) != LLVMGetTypeKind(pt)) {
        if (LLVMGetTypeKind(at) == LLVMIntegerTypeKind &&
            LLVMGetTypeKind(pt) == LLVMPointerTypeKind)
          args[i] = LLVMBuildIntToPtr(ctx->builder, args[i], pt, "itp");
        else if (LLVMGetTypeKind(at) == LLVMPointerTypeKind &&
                 LLVMGetTypeKind(pt) == LLVMIntegerTypeKind)
          args[i] = LLVMBuildPtrToInt(ctx->builder, args[i],
                                      LLVMInt64TypeInContext(ctx->lctx), "pti");
      }
    }
  }

  LLVMTypeRef  ret_t = LLVMGetReturnType(func_type);
  const char  *rname = (LLVMGetTypeKind(ret_t) == LLVMVoidTypeKind) ? "" : "call";

  LLVMValueRef result =
      LLVMBuildCall2(ctx->builder, func_type, func, args, (unsigned)nargs, rname);

  free(param_types);
  free(args);
  return result;
}

/* ── EXPR_BLOCK  { stmts...; tail_expr } ───────────────────────────────── */
static LLVMValueRef codegen_expr_block(CodegenContext *ctx, nova_expr_t *expr) {
  if (!expr->data.block) return NULL;
  int saved = codegen_scope_save(ctx);
  codegen_stmt(ctx, expr->data.block);
  codegen_scope_restore(ctx, saved);
  return LLVMConstNull(LLVMInt64TypeInContext(ctx->lctx)); /* void block */
}

/* ── Top-level expression dispatcher ───────────────────────────────────── */
static LLVMValueRef codegen_expr(CodegenContext *ctx, nova_expr_t *expr) {
  if (!expr) return NULL;

  switch (expr->kind) {
  case EXPR_INT:
  case EXPR_FLOAT:
  case EXPR_BOOL:       return codegen_expr_literal(ctx, expr);
  case EXPR_STR: {
    const char *s = expr->data.lit_str ? expr->data.lit_str : "";
    return LLVMBuildGlobalStringPtr(ctx->builder, s, "str");
  }
  case EXPR_IDENT:      return codegen_expr_ident(ctx, expr);
  case EXPR_ASSIGN:     return codegen_expr_assign(ctx, expr);
  case EXPR_BINARY:     return codegen_expr_binary(ctx, expr);
  case EXPR_CAST:       return codegen_expr_cast(ctx, expr);
  case EXPR_CALL:       return codegen_expr_call(ctx, expr);
  case EXPR_FIELD_ACCESS: return codegen_expr_field_access(ctx, expr);
  case EXPR_BLOCK:      return codegen_expr_block(ctx, expr);
  case EXPR_STRUCT_INIT:
    return LLVMConstNull(LLVMInt64TypeInContext(ctx->lctx));
  /* Graceful stubs for unimplemented but non-fatal nodes */
  case EXPR_ARRAY_LIT:
  case EXPR_INDEX:
  case EXPR_MATCH:
  case EXPR_ENUM_VARIANT:
  case EXPR_HEAP_NEW:
  case EXPR_ADDR_OF:
  case EXPR_DEREF:
  case EXPR_AWAIT:
    fprintf(stderr, "codegen: expr kind %d not yet implemented\n", expr->kind);
    return LLVMConstNull(LLVMInt64TypeInContext(ctx->lctx));
  default:
    fprintf(stderr, "codegen: unknown expr kind %d\n", expr->kind);
    return NULL;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Statement Code Generation
// ═══════════════════════════════════════════════════════════════════════════

static bool codegen_stmt_return(CodegenContext *ctx, nova_stmt_t *stmt) {
  if (stmt->data.return_expr) {
    LLVMValueRef v = codegen_expr(ctx, stmt->data.return_expr);
    if (!v) return false;
    /* Widen to declared return type if needed */
    if (ctx->current_ret_type &&
        LLVMTypeOf(v) != ctx->current_ret_type &&
        LLVMGetTypeKind(LLVMTypeOf(v)) == LLVMIntegerTypeKind &&
        LLVMGetTypeKind(ctx->current_ret_type) == LLVMIntegerTypeKind)
      v = LLVMBuildSExt(ctx->builder, v, ctx->current_ret_type, "rsext");
    LLVMBuildRet(ctx->builder, v);
  } else {
    LLVMBuildRetVoid(ctx->builder);
  }
  return true;
}

static bool codegen_stmt_var_decl(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMValueRef init_val = NULL;
  if (stmt->data.var_decl.init) {
    init_val = codegen_expr(ctx, stmt->data.var_decl.init);
    if (!init_val) return false;
  }

  /* Determine storage type: explicit > inferred from init > i64 fallback */
  LLVMTypeRef var_type;
  if (stmt->data.var_decl.type) {
    var_type = codegen_type(ctx, stmt->data.var_decl.type);
  } else if (init_val) {
    var_type = LLVMTypeOf(init_val);
  } else {
    var_type = LLVMInt64TypeInContext(ctx->lctx);
  }

  LLVMValueRef alloca =
      LLVMBuildAlloca(ctx->builder, var_type, stmt->data.var_decl.name);

  if (init_val) {
    /* Widen int if types mismatch (e.g. i1 bool stored in i64 slot) */
    LLVMTypeRef it = LLVMTypeOf(init_val);
    if (it != var_type &&
        LLVMGetTypeKind(it)       == LLVMIntegerTypeKind &&
        LLVMGetTypeKind(var_type) == LLVMIntegerTypeKind)
      init_val = LLVMBuildSExt(ctx->builder, init_val, var_type, "sext");
    LLVMBuildStore(ctx->builder, init_val, alloca);
  }

  codegen_add_symbol(ctx, stmt->data.var_decl.name, alloca, var_type);
  return true;
}

/* STMT_CHECK → if/else with optional else branch */
static bool codegen_stmt_if(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMValueRef cond = codegen_expr(ctx, stmt->data.check_stmt.condition);
  if (!cond) return false;

  /* Ensure i1 */
  if (LLVMGetTypeKind(LLVMTypeOf(cond)) != LLVMInt1TypeKind)
    cond = LLVMBuildICmp(ctx->builder, LLVMIntNE, cond,
                         LLVMConstNull(LLVMTypeOf(cond)), "tobool");

  LLVMValueRef fn      = ctx->current_function;
  LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "then");
  LLVMBasicBlockRef else_bb = stmt->data.check_stmt.else_branch
                                  ? LLVMAppendBasicBlockInContext(ctx->lctx, fn, "else")
                                  : NULL;
  LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "ifcont");

  LLVMBuildCondBr(ctx->builder, cond, then_bb,
                  else_bb ? else_bb : merge_bb);

  /* then */
  LLVMPositionBuilderAtEnd(ctx->builder, then_bb);
  int saved = codegen_scope_save(ctx);
  bool ok = codegen_stmt(ctx, stmt->data.check_stmt.then_branch);
  codegen_scope_restore(ctx, saved);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder)))
    LLVMBuildBr(ctx->builder, merge_bb);

  /* else (optional) */
  if (else_bb) {
    LLVMPositionBuilderAtEnd(ctx->builder, else_bb);
    saved = codegen_scope_save(ctx);
    ok &= codegen_stmt(ctx, stmt->data.check_stmt.else_branch);
    codegen_scope_restore(ctx, saved);
    if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder)))
      LLVMBuildBr(ctx->builder, merge_bb);
  }

  LLVMPositionBuilderAtEnd(ctx->builder, merge_bb);
  return ok;
}

/* STMT_WHILE */
static bool codegen_stmt_while(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMValueRef fn = ctx->current_function;

  LLVMBasicBlockRef cond_bb  = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "while.cond");
  LLVMBasicBlockRef body_bb  = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "while.body");
  LLVMBasicBlockRef after_bb = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "while.after");

  LLVMBuildBr(ctx->builder, cond_bb);

  /* condition block */
  LLVMPositionBuilderAtEnd(ctx->builder, cond_bb);
  LLVMValueRef cond = codegen_expr(ctx, stmt->data.while_stmt.condition);
  if (!cond) return false;
  if (LLVMGetTypeKind(LLVMTypeOf(cond)) != LLVMInt1TypeKind)
    cond = LLVMBuildICmp(ctx->builder, LLVMIntNE, cond,
                         LLVMConstNull(LLVMTypeOf(cond)), "tobool");
  LLVMBuildCondBr(ctx->builder, cond, body_bb, after_bb);

  /* body */
  LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
  int saved = codegen_scope_save(ctx);
  bool ok = codegen_stmt(ctx, stmt->data.while_stmt.body);
  codegen_scope_restore(ctx, saved);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder)))
    LLVMBuildBr(ctx->builder, cond_bb);

  LLVMPositionBuilderAtEnd(ctx->builder, after_bb);
  return ok;
}

/* STMT_FOR  for binding in start..end { body } */
static bool codegen_stmt_for(CodegenContext *ctx, nova_stmt_t *stmt) {
  LLVMTypeRef  i64  = LLVMInt64TypeInContext(ctx->lctx);
  LLVMValueRef fn   = ctx->current_function;

  /* Allocate loop counter */
  LLVMValueRef counter = LLVMBuildAlloca(ctx->builder, i64,
                                          stmt->data.for_stmt.binding);
  LLVMValueRef start_v = codegen_expr(ctx, stmt->data.for_stmt.start);
  if (!start_v) return false;
  if (LLVMTypeOf(start_v) != i64)
    start_v = LLVMBuildSExt(ctx->builder, start_v, i64, "sext");
  LLVMBuildStore(ctx->builder, start_v, counter);

  LLVMBasicBlockRef cond_bb  = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "for.cond");
  LLVMBasicBlockRef body_bb  = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "for.body");
  LLVMBasicBlockRef incr_bb  = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "for.incr");
  LLVMBasicBlockRef after_bb = LLVMAppendBasicBlockInContext(ctx->lctx, fn, "for.after");

  LLVMBuildBr(ctx->builder, cond_bb);

  /* condition: counter < end */
  LLVMPositionBuilderAtEnd(ctx->builder, cond_bb);
  LLVMValueRef end_v = codegen_expr(ctx, stmt->data.for_stmt.end);
  if (!end_v) return false;
  if (LLVMTypeOf(end_v) != i64)
    end_v = LLVMBuildSExt(ctx->builder, end_v, i64, "sext");
  LLVMValueRef cur  = LLVMBuildLoad2(ctx->builder, i64, counter, "cur");
  LLVMValueRef cond = LLVMBuildICmp(ctx->builder, LLVMIntSLT, cur, end_v, "forcond");
  LLVMBuildCondBr(ctx->builder, cond, body_bb, after_bb);

  /* body — expose binding as a read-only symbol */
  LLVMPositionBuilderAtEnd(ctx->builder, body_bb);
  int saved = codegen_scope_save(ctx);
  codegen_add_symbol(ctx, stmt->data.for_stmt.binding, counter, i64);
  bool ok = codegen_stmt(ctx, stmt->data.for_stmt.body);
  codegen_scope_restore(ctx, saved);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder)))
    LLVMBuildBr(ctx->builder, incr_bb);

  /* increment */
  LLVMPositionBuilderAtEnd(ctx->builder, incr_bb);
  LLVMValueRef next = LLVMBuildAdd(ctx->builder,
                          LLVMBuildLoad2(ctx->builder, i64, counter, "inc"),
                          LLVMConstInt(i64, 1, 0), "next");
  LLVMBuildStore(ctx->builder, next, counter);
  LLVMBuildBr(ctx->builder, cond_bb);

  LLVMPositionBuilderAtEnd(ctx->builder, after_bb);
  return ok;
}

static bool codegen_stmt_expr(CodegenContext *ctx, nova_stmt_t *stmt) {
  codegen_expr(ctx, stmt->data.expr); /* void call result is fine */
  return true;
}

static bool codegen_stmt_block(CodegenContext *ctx, nova_stmt_t *stmt) {
  int saved = codegen_scope_save(ctx);
  for (size_t i = 0; i < stmt->data.block.count; i++) {
    if (!codegen_stmt(ctx, stmt->data.block.statements[i])) {
      codegen_scope_restore(ctx, saved);
      return false;
    }
    /* Stop emitting after a terminator (return/break) */
    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder)))
      break;
  }
  codegen_scope_restore(ctx, saved);
  return true;
}

static bool codegen_stmt(CodegenContext *ctx, nova_stmt_t *stmt) {
  if (!stmt) return true;

  switch (stmt->kind) {
  case STMT_RETURN:      return codegen_stmt_return(ctx, stmt);
  case STMT_VAR_DECL:    return codegen_stmt_var_decl(ctx, stmt);
  case STMT_EXPR:        return codegen_stmt_expr(ctx, stmt);
  case STMT_BLOCK:       return codegen_stmt_block(ctx, stmt);
  case STMT_CHECK:       return codegen_stmt_if(ctx, stmt);
  case STMT_WHILE:       return codegen_stmt_while(ctx, stmt);
  case STMT_FOR:         return codegen_stmt_for(ctx, stmt);
  /* Declarations at statement level — skip IR, handled at top-level */
  case STMT_IMPORT:
  case STMT_STRUCT_DECL:
  case STMT_ENUM_DECL:
  case STMT_FOREIGN:     return true;
  /* Graceful stubs */
  case STMT_BREAK:
  case STMT_CONTINUE:
    fprintf(stderr, "codegen: break/continue outside loop not yet supported\n");
    return true;
  case STMT_EACH:
  case STMT_SPAWN:
  case STMT_TRY_CATCH:
  case STMT_YIELD:
    fprintf(stderr, "codegen: stmt kind %d not yet implemented\n", stmt->kind);
    return true;
  default:
    fprintf(stderr, "codegen: unknown stmt kind %d\n", stmt->kind);
    return false;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Function Code Generation
// ═══════════════════════════════════════════════════════════════════════════

static bool codegen_function(CodegenContext *ctx, nova_stmt_t *fn_stmt) {
  if (fn_stmt->kind != STMT_FN) return false;

  size_t       pc          = fn_stmt->data.fn_stmt.param_count;
  LLVMTypeRef *param_types = NULL;

  if (pc > 0) {
    param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * pc);
    if (!param_types) return false;
    for (size_t i = 0; i < pc; i++)
      param_types[i] = codegen_type(ctx, fn_stmt->data.fn_stmt.params[i]->type);
  }

  LLVMTypeRef ret_type =
      codegen_type(ctx, fn_stmt->data.fn_stmt.return_type);
  LLVMTypeRef func_type =
      LLVMFunctionType(ret_type, param_types, (unsigned)pc, /*variadic=*/0);

  LLVMValueRef func =
      LLVMAddFunction(ctx->module, fn_stmt->data.fn_stmt.name, func_type);

  ctx->current_function  = func;
  ctx->current_ret_type  = ret_type;

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(ctx->lctx, func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, entry);

  /* Allocate and store each parameter */
  for (size_t i = 0; i < pc; i++) {
    LLVMValueRef param = LLVMGetParam(func, (unsigned)i);
    const char  *pname = fn_stmt->data.fn_stmt.params[i]->name;
    LLVMSetValueName2(param, pname, strlen(pname));

    LLVMValueRef alloca = LLVMBuildAlloca(ctx->builder, param_types[i], pname);
    LLVMBuildStore(ctx->builder, param, alloca);
    codegen_add_symbol(ctx, pname, alloca, param_types[i]);
  }

  bool success = codegen_stmt(ctx, fn_stmt->data.fn_stmt.body);

  /* Implicit terminator */
  if (success) {
    LLVMBasicBlockRef last_bb = LLVMGetInsertBlock(ctx->builder);
    if (!LLVMGetBasicBlockTerminator(last_bb)) {
      if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind)
        LLVMBuildRetVoid(ctx->builder);
      else
        LLVMBuildRet(ctx->builder, LLVMConstNull(ret_type));
    }
  }

  free(param_types);
  return success;
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════

bool nova_llvm_codegen_program(NovaLLVMContext *llvm_ctx,
                               nova_program_t  *program) {
  if (!llvm_ctx || !program) return false;

  CodegenContext *ctx     = codegen_context_create(llvm_ctx);
  bool            success = true;

  for (size_t i = 0; i < program->declaration_count; i++) {
    nova_top_level_t *decl = program->declarations[i];
    if (!decl) continue;

    if (decl->kind == DECL_FUNCTION) {
      if (!codegen_function(ctx, (nova_stmt_t *)decl->data)) {
        fprintf(stderr, "codegen: failed on declaration %zu\n", i);
        success = false;
        break;
      }
    }
    /* DECL_STRUCT / DECL_ENUM / DECL_VAR / DECL_IMPORT — future work */
  }

  codegen_context_destroy(ctx);
  return success;
}

#else // !HAVE_LLVM

bool nova_llvm_codegen_program(NovaLLVMContext *llvm_ctx,
                               nova_program_t  *program) {
  (void)llvm_ctx; (void)program;
  fprintf(stderr, "LLVM support not compiled in\n");
  return false;
}

#endif // HAVE_LLVM
