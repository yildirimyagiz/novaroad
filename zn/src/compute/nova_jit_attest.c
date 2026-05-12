/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_jit_attest.c — JIT Compiler Attestation Checkpoint
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * JIT derleyicisi (CodeGen) bu modülü çağırır.
 *
 * Adımlar:
 * 1. JIT talebi -> Symbolic IR üret
 * 2. Solver ile Obligation'ları çöz
 * 3. Policy ile Verdict al (DENY ise derleme iptal)
 * 4. ALLOW ise Attestation Record kaydet
 * 5. CodeGen'e devam et
 */

#include "nova_attest.h"
#include "nova_errors.h"
#include "nova_graph_obligations.h" /* nova_graph_verify */
#include "nova_graph_structs.h"     /* NovaIRGraph */
#include "nova_jit.h"
#include "nova_obligation.h"
#include "nova_policy.h"
#include "nova_solver.h"
#include <stdio.h>
#include <stdlib.h>

extern PolicyConfig *g_jit_policy;
extern SolverSession *g_jit_solver;

int nova_jit_verify_and_attest(const char *kernel_name,
                                 const NovaIRGraph *graph) {
  if (!g_jit_policy || !g_jit_solver || !graph) {
    /* Policy disabled or not initialized -> FAIL SAFE */
    return ZEN_ERR_POLICY_VIOLATION;
  }

  /* 1. Generate Obligations from Graph + Contracts */
  ObligationSet *obl_set = nova_graph_verify(graph);
  if (!obl_set)
    return ZEN_ERR_KERNEL_INVALID; /* Was ZEN_ERR_OBLIGATION_Generate_Failed */

  /* 2. Solve Obligations */
  int solve_result = solver_discharge_all(g_jit_solver, obl_set);
  if (solve_result != 0) {
    /* Not all proved immediately */
    /* Policy engine decides what to do next */
  }

  /* 3. Policy Decision */
  VerdictReport report = policy_evaluate(g_jit_policy, obl_set);

  if (report.verdict == VERDICT_DENY) {
    fprintf(stderr, "⛔ JIT COMPILATION BLOCKED: %s\n", kernel_name);
    verdict_print(&report);

    obligation_set_destroy(obl_set);
    verdict_destroy(&report);
    return ZEN_ERR_EXECUTION_DENIED;
  }

  if (report.verdict == VERDICT_WARN) {
    fprintf(stderr, "⚠️  JIT PERMITTED WITH WARNINGS: %s\n", kernel_name);
  }

  /* 4. Attestation Recording (Sealing the Proof) */
  nova_attest_record(obl_set, &report, policy_mode_name(g_jit_policy->mode));

  /* Cleanup */
  verdict_destroy(&report);
  obligation_set_destroy(obl_set);

  return 0; /* Success - Proceed to CodeGen */
}
