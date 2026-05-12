#ifndef NOVA_SMT_IR_H
#define NOVA_SMT_IR_H

#include <stdbool.h>

/**
 * Normalized SMT-IR for Nova constraints.
 * Supports a subset of SMT-LIB v2 relevant to ML (Floating Point, Arrays,
 * BitVectors).
 */

typedef enum {
  SMT_TYPE_BOOL,
  SMT_TYPE_INT,
  SMT_TYPE_REAL,
  SMT_TYPE_FP16,
  SMT_TYPE_FP32,
  SMT_TYPE_ARRAY
} SmtType;

typedef enum {
  SMT_OP_EQ,
  SMT_OP_LT,
  SMT_OP_GT,
  SMT_OP_ADD,
  SMT_OP_SUB,
  SMT_OP_MUL,
  SMT_OP_AND,
  SMT_OP_OR,
  SMT_OP_NOT
} SmtOp;

typedef struct SmtExpr {
  SmtType type;
  SmtOp op;
  struct SmtExpr *lhs;
  struct SmtExpr *rhs;
  char *literal; // For variables or constants
} SmtExpr;

// Create SMT expressions
SmtExpr *nova_smt_create_var(SmtType type, const char *name);
SmtExpr *nova_smt_create_binop(SmtOp op, SmtExpr *lhs, SmtExpr *rhs);

#endif // NOVA_SMT_IR_H
