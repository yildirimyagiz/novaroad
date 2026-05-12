#ifndef NOVA_FORMAL_POLICY_H
#define NOVA_FORMAL_POLICY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../system/nova_common.h"
#include "nova_obligation.h"

typedef enum {
  Z_OPT_STRICT,    /* No unsafe transforms. Bit-exact. */
  Z_OPT_VERIFIED,  /* Transform only if proven safe.   */
  Z_OPT_AGGRESSIVE /* Max speed if invariants hold.    */
} NovaOptimizationLevel;

typedef enum {
  Z_DET_BIT_EXACT,         /* No reassociation, contraction etc. */
  Z_DET_APP_DETERMINISTIC, /* Logical equiv allowed (a+b vs b+a) */
  Z_DET_RELAXED            /* Non-deterministic reductions allowed */
} NovaDeterminismLevel;

typedef struct {
  NovaOptimizationLevel opt_level;
  NovaDeterminismLevel det_level;

  /* Feature toggles derived from levels & proofs */
  bool allow_reassoc;    // Requires (verified + det_app_det) OR aggressive
  bool allow_contract;   // FMA generation
  bool allow_reciprocal; // 1/x * y instead of y/x
  bool allow_vectorize;  // Requires memory_proven
  bool allow_no_nans;    // Requires no_nans proof
  bool allow_no_infs;    // Requires no_infs proof

  /* Advanced Memory Optimizations */
  bool allow_bounds_elision; // Skip runtime index checks (Requires ShapeSafe)
  bool allow_ptr_noalias; // Arg pointers don't overlap (Requires AliasingFree)
  bool assume_ptr_align;  // Pointers are SIMD-aligned (Requires MemorySafe)
} NovaOptimizationConfig;

typedef enum {
    POLICY_STRICT,
    POLICY_GUARDED,
    POLICY_PERMISSIVE,
    POLICY_DISABLED
} PolicyMode;

typedef struct {
    ObligationKind kind;
    bool required;
    bool blocking;
    const char *description;
} PolicyRule;

typedef struct {
    PolicyMode mode;
    bool enforce_proofs;
    bool audit_only;
    bool quarantine_on_refutation;
    PolicyRule *rules;
    size_t rule_count;
    size_t rule_capacity;
    bool enable_audit_log;
    const char *audit_log_path;
    int grace_period_ms;
    const char **trusted_kernels;
    size_t trusted_count;
} PolicyConfig;

const char *policy_mode_name(PolicyMode mode);
const char *verdict_name(ExecutionVerdict verdict);

void policy_global_init(void);

PolicyConfig *policy_create(PolicyMode mode);
void policy_destroy(PolicyConfig *config);
void policy_add_rule(PolicyConfig *config, ObligationKind kind, bool required, bool blocking, const char *description);
void policy_load_nova_defaults(PolicyConfig *config);
void policy_trust_kernel(PolicyConfig *config, const char *kernel_name);

VerdictReport policy_evaluate(const PolicyConfig *config, const ObligationSet *obligations);
VerdictReport policy_evaluate_proposition(const char *proposition);

void verdict_print(const VerdictReport *report);
void verdict_audit_log(const PolicyConfig *config, const VerdictReport *report);
int verdict_export_json(const VerdictReport *report, const char *path);
void verdict_destroy(VerdictReport *report);

NovaOptimizationConfig nova_policy_derive_optimization(NovaOptimizationLevel opt, NovaDeterminismLevel det, bool is_verified, bool is_memory_proven);

#endif
