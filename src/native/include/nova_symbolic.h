/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA SYMBOLIC IR — Kernel → Mantıksal Fonksiyon Dönüşüm Motoru
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * "Kod kopyalamazsın, semantik çıkarırsın."
 *
 * C kodu:     out[i] = a[i] + b[i]
 * Symbolic:   ∀i → outᵢ = aᵢ + bᵢ
 *
 * Bu çeviri motoru = Nova'in gerçek "Gödel çekirdeği".
 * Bu olmadan formal verification kozmetik kalır.
 */

#ifndef NOVA_SYMBOLIC_H
#define NOVA_SYMBOLIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * SYMBOLIC TYPES — Mantıksal tip sistemi
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  SYM_TYPE_BOOL,   /* Boolean: true / false          */
  SYM_TYPE_INT,    /* Arbitrary precision integer     */
  SYM_TYPE_REAL,   /* Mathematical real               */
  SYM_TYPE_BITVEC, /* Bit-vector (32/64-bit)          */
  SYM_TYPE_FLOAT,  /* IEEE 754 floating point         */
  SYM_TYPE_ARRAY,  /* SMT array: (Array Int Real)     */
  SYM_TYPE_TENSOR, /* Multi-dim array with shape      */
} SymType;

typedef struct {
  SymType base_type;
  int bitvec_width;  /* for SYM_TYPE_BITVEC: 32 or 64  */
  int float_width;   /* for SYM_TYPE_FLOAT: 16/32/64   */
  int *tensor_shape; /* for SYM_TYPE_TENSOR             */
  int tensor_ndim;
} SymTypeInfo;

/* ═══════════════════════════════════════════════════════════════════════════
 * SYMBOLIC EXPRESSIONS — AST of logical formulas
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  /* Terminals */
  SYM_CONST_INT,  /* Sabit tam sayı                  */
  SYM_CONST_REAL, /* Sabit ondalık                   */
  SYM_CONST_BOOL, /* true / false                    */
  SYM_VAR,        /* Serbest değişken                */

  /* Arithmetic */
  SYM_ADD, /* a + b                           */
  SYM_SUB, /* a - b                           */
  SYM_MUL, /* a × b                           */
  SYM_DIV, /* a / b                           */
  SYM_MOD, /* a mod b                         */
  SYM_NEG, /* -a                              */
  SYM_ABS, /* |a|                             */

  /* Comparison */
  SYM_EQ,  /* a = b                           */
  SYM_NEQ, /* a ≠ b                           */
  SYM_LT,  /* a < b                           */
  SYM_LE,  /* a ≤ b                           */
  SYM_GT,  /* a > b                           */
  SYM_GE,  /* a ≥ b                           */

  /* Logic */
  SYM_AND,     /* a ∧ b                           */
  SYM_OR,      /* a ∨ b                           */
  SYM_NOT,     /* ¬a                              */
  SYM_IMPLIES, /* a ⇒ b                           */
  SYM_IFF,     /* a ⇔ b                           */

  /* Quantifiers */
  SYM_FORALL, /* ∀x. P(x)                        */
  SYM_EXISTS, /* ∃x. P(x)                        */

  /* Array / Tensor */
  SYM_SELECT, /* (select array index)             */
  SYM_STORE,  /* (store array index value)        */
  SYM_DIM,    /* tensor dimension query           */

  /* Special */
  SYM_ITE,   /* if-then-else                    */
  SYM_LET,   /* let binding                     */
  SYM_APPLY, /* function application            */

  /* Floating point safety */
  SYM_IS_NAN,  /* isNaN(x)                        */
  SYM_IS_INF,  /* isInf(x)                        */
  SYM_FP_SAFE, /* ¬isNaN(x) ∧ ¬isInf(x)          */
} SymExprKind;

typedef struct SymExpr {
  SymExprKind kind;
  SymTypeInfo type;

  /* Terminal values */
  int64_t int_val;
  double real_val;
  bool bool_val;
  const char *var_name;

  /* Children for composite expressions */
  struct SymExpr *left;
  struct SymExpr *right;
  struct SymExpr *third; /* for ITE and FORALL body */

  /* Quantifier variable */
  const char *bound_var;
  struct SymExpr *bound_lo; /* Quantifier lower bound */
  struct SymExpr *bound_hi; /* Quantifier upper bound */

  /* Source tracking */
  const char *source_file;
  int source_line;
  uint64_t expr_id;
} SymExpr;

/* ═══════════════════════════════════════════════════════════════════════════
 * SYMBOLIC CONTEXT — Kernel'in mantıksal temsili
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  const char *kernel_name;

  /* Declarations: symbolic variables representing inputs/outputs */
  SymExpr **declarations;
  size_t decl_count;
  size_t decl_capacity;

  /* Assertions: conditions that must hold */
  SymExpr **assertions;
  size_t assert_count;
  size_t assert_capacity;

  /* Assumptions: preconditions (assumed true) */
  SymExpr **assumptions;
  size_t assume_count;
  size_t assume_capacity;

  /* Expression pool (arena-style allocation) */
  SymExpr *expr_pool;
  size_t pool_used;
  size_t pool_capacity;
  uint64_t next_expr_id;
} SymContext;

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTEXT API
 * ═══════════════════════════════════════════════════════════════════════════
 */

SymContext *sym_context_create(const char *kernel_name);
void sym_context_destroy(SymContext *ctx);

/* ═══════════════════════════════════════════════════════════════════════════
 * EXPRESSION BUILDERS
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Terminals */
SymExpr *sym_const_int(SymContext *ctx, int64_t value);
SymExpr *sym_const_real(SymContext *ctx, double value);
SymExpr *sym_const_bool(SymContext *ctx, bool value);
SymExpr *sym_var(SymContext *ctx, const char *name, SymType type);
SymExpr *sym_var_bitvec(SymContext *ctx, const char *name, int width);
SymExpr *sym_var_tensor(SymContext *ctx, const char *name, int *shape, int ndim,
                        SymType elem_type);

/* Arithmetic */
SymExpr *sym_add(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_sub(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_mul(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_div(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_neg(SymContext *ctx, SymExpr *a);
SymExpr *sym_abs(SymContext *ctx, SymExpr *a);

/* Comparison */
SymExpr *sym_eq(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_lt(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_le(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_gt(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_ge(SymContext *ctx, SymExpr *a, SymExpr *b);

/* Logic */
SymExpr *sym_and(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_or(SymContext *ctx, SymExpr *a, SymExpr *b);
SymExpr *sym_not(SymContext *ctx, SymExpr *a);
SymExpr *sym_implies(SymContext *ctx, SymExpr *a, SymExpr *b);

/* Quantifiers:
 *   ∀i ∈ [lo, hi). body(i)
 */
SymExpr *sym_forall(SymContext *ctx, const char *var, SymExpr *lo, SymExpr *hi,
                    SymExpr *body);
SymExpr *sym_exists(SymContext *ctx, const char *var, SymExpr *lo, SymExpr *hi,
                    SymExpr *body);

/* Array / Tensor */
SymExpr *sym_select(SymContext *ctx, SymExpr *array, SymExpr *index);
SymExpr *sym_store(SymContext *ctx, SymExpr *array, SymExpr *index,
                   SymExpr *value);
SymExpr *sym_dim(SymContext *ctx, SymExpr *tensor, int axis);

/* Floating point safety */
SymExpr *sym_is_nan(SymContext *ctx, SymExpr *x);
SymExpr *sym_is_inf(SymContext *ctx, SymExpr *x);
SymExpr *sym_fp_safe(SymContext *ctx, SymExpr *x);

/* ITE */
SymExpr *sym_ite(SymContext *ctx, SymExpr *cond, SymExpr *then_e,
                 SymExpr *else_e);

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTEXT OPERATIONS
 * ═══════════════════════════════════════════════════════════════════════════
 */

/** Declare a symbolic variable in this context */
void sym_declare(SymContext *ctx, SymExpr *var_expr);

/** Assert a condition that must be proved */
void sym_assert(SymContext *ctx, SymExpr *condition);

/** Assume a precondition (taken as axiom) */
void sym_assume(SymContext *ctx, SymExpr *condition);

/* ═══════════════════════════════════════════════════════════════════════════
 * PRETTY PRINTING
 * ═══════════════════════════════════════════════════════════════════════════
 */

/** Pretty-print expression to buffer */
int sym_expr_to_string(const SymExpr *expr, char *buf, size_t buf_size);

/** Pretty-print entire context */
/** Pretty-print entire context */
void sym_context_dump(const SymContext *ctx);

/* ═══════════════════════════════════════════════════════════════════════════
 * SMT-LIB2 EXPORT
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Emit full SMT-LIB2 problem for the context
 * Returns number of bytes written or -1 on error
 */
int sym_emit_smt2(const SymContext *ctx, char *buf, size_t buf_size);

/**
 * Write SMT-LIB2 problem to file
 */
int sym_emit_smt2_file(const SymContext *ctx, const char *path);

/**
 * Emit single expression in SMT-LIB2 format
 */
int sym_expr_to_smt2(const SymExpr *expr, char *buf, size_t buf_size);

#endif /* NOVA_SYMBOLIC_H */
