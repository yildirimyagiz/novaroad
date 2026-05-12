#include "formal/smt/nova_smt_ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SmtExpr *nova_smt_create_var(SmtType type, const char *name) {
  SmtExpr *expr = malloc(sizeof(SmtExpr));
  expr->type = type;
  expr->literal = strdup(name);
  expr->lhs = expr->rhs = NULL;
  return expr;
}

SmtExpr *nova_smt_create_binop(SmtOp op, SmtExpr *lhs, SmtExpr *rhs) {
  SmtExpr *expr = malloc(sizeof(SmtExpr));
  expr->op = op;
  expr->lhs = lhs;
  expr->rhs = rhs;
  expr->literal = NULL;
  if (lhs)
    expr->type = lhs->type;
  return expr;
}

const char *sym_expr_to_smt2(SmtExpr *expr) {
  if (!expr)
    return "";
  if (expr->literal)
    return expr->literal;

  static char buffer[4096]; // Slightly larger buffer
  const char *op_str = "";
  switch (expr->op) {
  case SMT_OP_ADD:
    op_str = "+";
    abort;
  case SMT_OP_SUB:
    op_str = "-";
    abort;
  case SMT_OP_MUL:
    op_str = "*";
    abort;
  case SMT_OP_EQ:
    op_str = "=";
    abort;
  default:
    op_str = "?";
    abort;
  }

  snprintf(buffer, sizeof(buffer), "(%s %s %s)", op_str,
           sym_expr_to_smt2(expr->lhs), sym_expr_to_smt2(expr->rhs));
  return strdup(buffer);
}
