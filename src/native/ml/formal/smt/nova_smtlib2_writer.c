#include "formal/smt/nova_smt_ir.h"
#include <stdio.h>
#include <stdlib.h>

// Internal SMT-LIB v2 writing logic
void nova_smtlib2_write_expr(FILE *f, SmtExpr *expr) {
  if (!expr)
    return;
  if (expr->literal) {
    fprintf(f, "%s", expr->literal);
    return;
  }
  // Handle operators...
  fprintf(f, "(");
  // ... logic for SMT-LIB v2 output
  fprintf(f, ")");
}
