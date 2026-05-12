/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA SOLVER BRIDGE — SMT Solver Soyutlama Katmanı
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * SymbolicIR → SMT-LIB2 → Solver → ProofReceipt
 *
 * Desteklenen solver'lar:
 *   1. CVC5 (birincil — Decision Procedures kitabı yazarları)
 *   2. Z3 (yedek — Microsoft Research)
 *   3. Fallback (basit rule-based checker)
 */

#ifndef NOVA_SOLVER_H
#define NOVA_SOLVER_H

#include "nova_obligation.h"
#include "nova_symbolic.h"
#include <stdbool.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * SOLVER BACKEND
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  SOLVER_CVC5,     /* cvc5 — primary                  */
  SOLVER_Z3,       /* Z3 — secondary                  */
  SOLVER_FALLBACK, /* Built-in rule engine             */
  SOLVER_AUTO,     /* Try cvc5 → z3 → fallback        */
} SolverBackend;

typedef enum {
  SOLVER_RESULT_SAT,     /* Satisfiable (counterexample found) */
  SOLVER_RESULT_UNSAT,   /* Unsatisfiable (proof holds)        */
  SOLVER_RESULT_UNKNOWN, /* Solver couldn't decide             */
  SOLVER_RESULT_TIMEOUT, /* Exceeded time limit                */
  SOLVER_RESULT_ERROR,   /* Solver error                       */
} SolverResult;

/* ═══════════════════════════════════════════════════════════════════════════
 * SOLVER CONFIGURATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  SolverBackend backend;
  double timeout_ms;     /* Per-obligation timeout         */
  bool produce_models;   /* Get counterexamples on SAT     */
  bool produce_proofs;   /* Get proof certificates on UNSAT*/
  bool incremental;      /* Use incremental solving        */
  int verbosity;         /* 0=silent, 1=summary, 2=detail  */
  const char *cvc5_path; /* Path to cvc5 binary            */
  const char *z3_path;   /* Path to z3 binary              */
} SolverConfig;

/** Default solver configuration */
SolverConfig solver_config_default(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * SOLVER SESSION
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct SolverSession SolverSession;

/** Create a solver session */
SolverSession *solver_session_create(const SolverConfig *config);

/** Destroy solver session */
void solver_session_destroy(SolverSession *session);

/* ═══════════════════════════════════════════════════════════════════════════
 * CORE API — Obligation çözümleme
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Tek bir obligation'ı çöz
 *
 * obligation.smt2_formula → solver → ProofReceipt
 */
ProofReceipt solver_check_obligation(SolverSession *session,
                                     Obligation *obligation);

/**
 * Bir ObligationSet'in tüm borçlarını çöz
 *
 * Her obligation paralel veya sıralı çözülebilir.
 * Tüm borçlar çözülünce set.all_proved güncellenir.
 */
int solver_discharge_all(SolverSession *session, ObligationSet *set);

/**
 * SymContext'ten otomatik obligation'lar üret ve çöz
 *
 * Context'teki her assertion bir obligation olur.
 */
ObligationSet *solver_verify_context(SolverSession *session,
                                     SymContext *sym_ctx);

/* ═══════════════════════════════════════════════════════════════════════════
 * SMT-LIB2 I/O
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * SymExpr → SMT-LIB2 string
 *
 * Örnek çıktı:
 *   (declare-const a (Array Int Real))
 *   (assert (forall ((i Int))
 *     (=> (and (>= i 0) (< i n))
 *         (= (select out i) (+ (select a i) (select b i))))))
 *   (check-sat)
 */
int sym_emit_smt2(const SymContext *ctx, char *buf, size_t buf_size);

/**
 * SMT-LIB2'yi dosyaya yaz
 */
int sym_emit_smt2_file(const SymContext *ctx, const char *path);

/**
 * SymExpr'i SMT-LIB2 term'e çevir
 */
int sym_expr_to_smt2(const SymExpr *expr, char *buf, size_t buf_size);

/* ═══════════════════════════════════════════════════════════════════════════
 * SOLVER STATISTICS
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  size_t total_queries;
  size_t sat_count;
  size_t unsat_count;
  size_t unknown_count;
  size_t timeout_count;
  size_t error_count;
  double total_solve_ms;
  double avg_solve_ms;
  double max_solve_ms;
  size_t cache_hits;
} SolverStats;

SolverStats solver_get_stats(const SolverSession *session);
void solver_print_stats(const SolverSession *session);

#endif /* NOVA_SOLVER_H */
