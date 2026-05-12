#ifndef NOVA_CVC5_BRIDGE_H
#define NOVA_CVC5_BRIDGE_H

#include "nova_smt_ir.h"

/**
 * Interface to cvc5 SMT solver.
 */

typedef enum {
  NOVA_SMT_SAT,
  NOVA_SMT_UNSAT,
  NOVA_SMT_UNKNOWN,
  NOVA_SMT_ERROR
} NovaSmtResult;

// Solve a set of constraints using cvc5
NovaSmtResult nova_cvc5_solve(SmtExpr **constraints, int count);

// Export constraints to SMT-LIB v2 format
char *nova_smtlib2_export(SmtExpr **constraints, int count);

#endif // NOVA_CVC5_BRIDGE_H
