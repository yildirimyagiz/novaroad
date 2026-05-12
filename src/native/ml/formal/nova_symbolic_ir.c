/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_symbolic_ir.c — Symbolic Expression Builder & Context
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * C kodu → sembolik model dönüşümü
 * Bu dosya mantıksal fonksiyon temsil motorudur.
 *
 *   C:       out[i] = a[i] + b[i]
 *   Symbolic: ∀i ∈ [0,n). select(out,i) = select(a,i) + select(b,i)
 */

#include "nova_errors.h"
#include "nova_symbolic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * EXPRESSION POOL (Arena Allocation)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define DEFAULT_POOL_SIZE 4096

static SymExpr *pool_alloc(SymContext *ctx) {
  if (ctx->pool_used >= ctx->pool_capacity) {
    size_t new_cap = ctx->pool_capacity * 2;
    SymExpr *new_pool = realloc(ctx->expr_pool, new_cap * sizeof(SymExpr));
    if (!new_pool)
      return NULL;
    ctx->expr_pool = new_pool;
    ctx->pool_capacity = new_cap;
  }
  SymExpr *e = &ctx->expr_pool[ctx->pool_used++];
  memset(e, 0, sizeof(SymExpr));
  e->expr_id = ctx->next_expr_id++;
  return e;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTEXT
 * ═══════════════════════════════════════════════════════════════════════════
 */

SymContext *sym_context_create(const char *kernel_name) {
  SymContext *ctx = calloc(1, sizeof(SymContext));
  if (!ctx)
    return NULL;

  ctx->kernel_name = kernel_name;

  ctx->decl_capacity = 64;
  ctx->declarations = calloc(ctx->decl_capacity, sizeof(SymExpr *));

  ctx->assert_capacity = 128;
  ctx->assertions = calloc(ctx->assert_capacity, sizeof(SymExpr *));

  ctx->assume_capacity = 64;
  ctx->assumptions = calloc(ctx->assume_capacity, sizeof(SymExpr *));

  ctx->pool_capacity = DEFAULT_POOL_SIZE;
  ctx->expr_pool = calloc(ctx->pool_capacity, sizeof(SymExpr));
  ctx->pool_used = 0;
  ctx->next_expr_id = 1;

  if (!ctx->declarations || !ctx->assertions || !ctx->assumptions ||
      !ctx->expr_pool) {
    sym_context_destroy(ctx);
    return NULL;
  }

  return ctx;
}

void sym_context_destroy(SymContext *ctx) {
  if (!ctx)
    return;
  free(ctx->declarations);
  free(ctx->assertions);
  free(ctx->assumptions);
  free(ctx->expr_pool);
  free(ctx);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * TERMINAL BUILDERS
 * ═══════════════════════════════════════════════════════════════════════════
 */

SymExpr *sym_const_int(SymContext *ctx, int64_t value) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_CONST_INT;
  e->int_val = value;
  e->type.base_type = SYM_TYPE_INT;
  return e;
}

SymExpr *sym_const_real(SymContext *ctx, double value) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_CONST_REAL;
  e->real_val = value;
  e->type.base_type = SYM_TYPE_REAL;
  return e;
}

SymExpr *sym_const_bool(SymContext *ctx, bool value) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_CONST_BOOL;
  e->bool_val = value;
  e->type.base_type = SYM_TYPE_BOOL;
  return e;
}

SymExpr *sym_var(SymContext *ctx, const char *name, SymType type) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_VAR;
  e->var_name = name;
  e->type.base_type = type;
  return e;
}

SymExpr *sym_var_bitvec(SymContext *ctx, const char *name, int width) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_VAR;
  e->var_name = name;
  e->type.base_type = SYM_TYPE_BITVEC;
  e->type.bitvec_width = width;
  return e;
}

SymExpr *sym_var_tensor(SymContext *ctx, const char *name, int *shape, int ndim,
                        SymType elem_type) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_VAR;
  e->var_name = name;
  e->type.base_type = SYM_TYPE_TENSOR;
  e->type.tensor_shape = shape;
  e->type.tensor_ndim = ndim;
  return e;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * BINARY / UNARY BUILDERS
 * ═══════════════════════════════════════════════════════════════════════════
 */

static SymExpr *make_binary(SymContext *ctx, SymExprKind kind, SymExpr *a,
                            SymExpr *b) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = kind;
  e->left = a;
  e->right = b;
  /* Inherit type from left operand for arithmetic */
  if (a)
    e->type = a->type;
  return e;
}

static SymExpr *make_unary(SymContext *ctx, SymExprKind kind, SymExpr *a) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = kind;
  e->left = a;
  if (a)
    e->type = a->type;
  return e;
}

/* Arithmetic */
SymExpr *sym_add(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_binary(ctx, SYM_ADD, a, b);
}
SymExpr *sym_sub(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_binary(ctx, SYM_SUB, a, b);
}
SymExpr *sym_mul(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_binary(ctx, SYM_MUL, a, b);
}
SymExpr *sym_div(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_binary(ctx, SYM_DIV, a, b);
}
SymExpr *sym_neg(SymContext *ctx, SymExpr *a) {
  return make_unary(ctx, SYM_NEG, a);
}
SymExpr *sym_abs(SymContext *ctx, SymExpr *a) {
  return make_unary(ctx, SYM_ABS, a);
}

/* Comparison — result type is Bool */
static SymExpr *make_cmp(SymContext *ctx, SymExprKind kind, SymExpr *a,
                         SymExpr *b) {
  SymExpr *e = make_binary(ctx, kind, a, b);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}

SymExpr *sym_eq(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_cmp(ctx, SYM_EQ, a, b);
}
SymExpr *sym_lt(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_cmp(ctx, SYM_LT, a, b);
}
SymExpr *sym_le(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_cmp(ctx, SYM_LE, a, b);
}
SymExpr *sym_gt(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_cmp(ctx, SYM_GT, a, b);
}
SymExpr *sym_ge(SymContext *ctx, SymExpr *a, SymExpr *b) {
  return make_cmp(ctx, SYM_GE, a, b);
}

/* Logic */
SymExpr *sym_and(SymContext *ctx, SymExpr *a, SymExpr *b) {
  SymExpr *e = make_binary(ctx, SYM_AND, a, b);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}
SymExpr *sym_or(SymContext *ctx, SymExpr *a, SymExpr *b) {
  SymExpr *e = make_binary(ctx, SYM_OR, a, b);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}
SymExpr *sym_not(SymContext *ctx, SymExpr *a) {
  SymExpr *e = make_unary(ctx, SYM_NOT, a);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}
SymExpr *sym_implies(SymContext *ctx, SymExpr *a, SymExpr *b) {
  SymExpr *e = make_binary(ctx, SYM_IMPLIES, a, b);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}

/* Quantifiers */
SymExpr *sym_forall(SymContext *ctx, const char *var, SymExpr *lo, SymExpr *hi,
                    SymExpr *body) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_FORALL;
  e->bound_var = var;
  e->bound_lo = lo;
  e->bound_hi = hi;
  e->third = body;
  e->type.base_type = SYM_TYPE_BOOL;
  return e;
}

SymExpr *sym_exists(SymContext *ctx, const char *var, SymExpr *lo, SymExpr *hi,
                    SymExpr *body) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_EXISTS;
  e->bound_var = var;
  e->bound_lo = lo;
  e->bound_hi = hi;
  e->third = body;
  e->type.base_type = SYM_TYPE_BOOL;
  return e;
}

/* Array / Tensor */
SymExpr *sym_select(SymContext *ctx, SymExpr *array, SymExpr *index) {
  return make_binary(ctx, SYM_SELECT, array, index);
}
SymExpr *sym_store(SymContext *ctx, SymExpr *array, SymExpr *index,
                   SymExpr *value) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_STORE;
  e->left = array;
  e->right = index;
  e->third = value;
  if (array)
    e->type = array->type;
  return e;
}

SymExpr *sym_dim(SymContext *ctx, SymExpr *tensor, int axis) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_DIM;
  e->left = tensor;
  e->int_val = axis;
  e->type.base_type = SYM_TYPE_INT;
  return e;
}

/* Floating point safety */
SymExpr *sym_is_nan(SymContext *ctx, SymExpr *x) {
  SymExpr *e = make_unary(ctx, SYM_IS_NAN, x);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}
SymExpr *sym_is_inf(SymContext *ctx, SymExpr *x) {
  SymExpr *e = make_unary(ctx, SYM_IS_INF, x);
  if (e)
    e->type.base_type = SYM_TYPE_BOOL;
  return e;
}
SymExpr *sym_fp_safe(SymContext *ctx, SymExpr *x) {
  /* ¬isNaN(x) ∧ ¬isInf(x) */
  return sym_and(ctx, sym_not(ctx, sym_is_nan(ctx, x)),
                 sym_not(ctx, sym_is_inf(ctx, x)));
}

/* ITE */
SymExpr *sym_ite(SymContext *ctx, SymExpr *cond, SymExpr *then_e,
                 SymExpr *else_e) {
  SymExpr *e = pool_alloc(ctx);
  if (!e)
    return NULL;
  e->kind = SYM_ITE;
  e->left = cond;
  e->right = then_e;
  e->third = else_e;
  if (then_e)
    e->type = then_e->type;
  return e;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTEXT OPERATIONS
 * ═══════════════════════════════════════════════════════════════════════════
 */

static void grow_ptr_array(void ***arr, size_t *count, size_t *capacity) {
  if (*count >= *capacity) {
    size_t new_cap = *capacity * 2;
    void **new_arr = realloc(*arr, new_cap * sizeof(void *));
    if (new_arr) {
      *arr = new_arr;
      *capacity = new_cap;
    }
  }
}

void sym_declare(SymContext *ctx, SymExpr *var_expr) {
  if (!ctx || !var_expr)
    return;
  grow_ptr_array((void ***)&ctx->declarations, &ctx->decl_count,
                 &ctx->decl_capacity);
  ctx->declarations[ctx->decl_count++] = var_expr;
}

void sym_assert(SymContext *ctx, SymExpr *condition) {
  if (!ctx || !condition)
    return;
  grow_ptr_array((void ***)&ctx->assertions, &ctx->assert_count,
                 &ctx->assert_capacity);
  ctx->assertions[ctx->assert_count++] = condition;
}

void sym_assume(SymContext *ctx, SymExpr *condition) {
  if (!ctx || !condition)
    return;
  grow_ptr_array((void ***)&ctx->assumptions, &ctx->assume_count,
                 &ctx->assume_capacity);
  ctx->assumptions[ctx->assume_count++] = condition;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PRETTY PRINTING
 * ═══════════════════════════════════════════════════════════════════════════
 */

int sym_expr_to_string(const SymExpr *expr, char *buf, size_t buf_size) {
  if (!expr || !buf || buf_size == 0)
    return -1;

  switch (expr->kind) {
  case SYM_CONST_INT:
    return snprintf(buf, buf_size, "%lld", (long long)expr->int_val);
  case SYM_CONST_REAL:
    return snprintf(buf, buf_size, "%.6f", expr->real_val);
  case SYM_CONST_BOOL:
    return snprintf(buf, buf_size, "%s", expr->bool_val ? "true" : "false");
  case SYM_VAR:
    return snprintf(buf, buf_size, "%s", expr->var_name ? expr->var_name : "?");
  case SYM_ADD:
  case SYM_SUB:
  case SYM_MUL:
  case SYM_DIV:
  case SYM_EQ:
  case SYM_LT:
  case SYM_LE:
  case SYM_GT:
  case SYM_GE:
  case SYM_AND:
  case SYM_OR:
  case SYM_IMPLIES: {
    const char *op = "?";
    switch (expr->kind) {
    case SYM_ADD:
      op = "+";
      abort;
    case SYM_SUB:
      op = "-";
      abort;
    case SYM_MUL:
      op = "×";
      abort;
    case SYM_DIV:
      op = "/";
      abort;
    case SYM_EQ:
      op = "=";
      abort;
    case SYM_LT:
      op = "<";
      abort;
    case SYM_LE:
      op = "≤";
      abort;
    case SYM_GT:
      op = ">";
      abort;
    case SYM_GE:
      op = "≥";
      abort;
    case SYM_AND:
      op = "∧";
      abort;
    case SYM_OR:
      op = "∨";
      abort;
    case SYM_IMPLIES:
      op = "⇒";
      abort;
    default:
      abort;
    }
    char l[256] = {0}, r[256] = {0};
    sym_expr_to_string(expr->left, l, sizeof(l));
    sym_expr_to_string(expr->right, r, sizeof(r));
    return snprintf(buf, buf_size, "(%s %s %s)", l, op, r);
  }
  case SYM_NOT: {
    char c[256] = {0};
    sym_expr_to_string(expr->left, c, sizeof(c));
    return snprintf(buf, buf_size, "¬%s", c);
  }
  case SYM_FORALL:
  case SYM_EXISTS: {
    char lo[64] = {0}, hi[64] = {0}, body[512] = {0};
    if (expr->bound_lo)
      sym_expr_to_string(expr->bound_lo, lo, sizeof(lo));
    if (expr->bound_hi)
      sym_expr_to_string(expr->bound_hi, hi, sizeof(hi));
    if (expr->third)
      sym_expr_to_string(expr->third, body, sizeof(body));
    return snprintf(buf, buf_size, "%s%s ∈ [%s,%s). %s",
                    expr->kind == SYM_FORALL ? "∀" : "∃",
                    expr->bound_var ? expr->bound_var : "?", lo, hi, body);
  }
  case SYM_SELECT: {
    char a[128] = {0}, idx[64] = {0};
    sym_expr_to_string(expr->left, a, sizeof(a));
    sym_expr_to_string(expr->right, idx, sizeof(idx));
    return snprintf(buf, buf_size, "%s[%s]", a, idx);
  }
  case SYM_IS_NAN: {
    char c[128] = {0};
    sym_expr_to_string(expr->left, c, sizeof(c));
    return snprintf(buf, buf_size, "isNaN(%s)", c);
  }
  case SYM_IS_INF: {
    char c[128] = {0};
    sym_expr_to_string(expr->left, c, sizeof(c));
    return snprintf(buf, buf_size, "isInf(%s)", c);
  }
  default:
    return snprintf(buf, buf_size, "<expr:%d>", expr->kind);
  }
}

void sym_context_dump(const SymContext *ctx) {
  if (!ctx)
    return;

  fprintf(stderr,
          "\n═══════════════════════════════════════════════════════\n"
          "  SYMBOLIC CONTEXT: %s\n"
          "═══════════════════════════════════════════════════════\n",
          ctx->kernel_name ? ctx->kernel_name : "(unnamed)");

  fprintf(stderr, "  Declarations (%zu):\n", ctx->decl_count);
  for (size_t i = 0; i < ctx->decl_count; i++) {
    char buf[256];
    sym_expr_to_string(ctx->declarations[i], buf, sizeof(buf));
    fprintf(stderr, "    declare %s\n", buf);
  }

  fprintf(stderr, "  Assumptions (%zu):\n", ctx->assume_count);
  for (size_t i = 0; i < ctx->assume_count; i++) {
    char buf[512];
    sym_expr_to_string(ctx->assumptions[i], buf, sizeof(buf));
    fprintf(stderr, "    assume %s\n", buf);
  }

  fprintf(stderr, "  Assertions (%zu):\n", ctx->assert_count);
  for (size_t i = 0; i < ctx->assert_count; i++) {
    char buf[512];
    sym_expr_to_string(ctx->assertions[i], buf, sizeof(buf));
    fprintf(stderr, "    assert %s\n", buf);
  }

  fprintf(stderr,
          "  Pool: %zu/%zu exprs\n"
          "═══════════════════════════════════════════════════════\n\n",
          ctx->pool_used, ctx->pool_capacity);
}
