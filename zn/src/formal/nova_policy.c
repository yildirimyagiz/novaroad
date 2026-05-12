/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_policy.c — Execution Permission Engine
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * "Program doğru mu?" değil, "Program çalışmaya izinli mi?"
 *
 * Bu dosya, kanıt durumuna göre kernel çalıştırma izni verir veya reddeder.
 */

#include "nova_policy.h"
#include "nova_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * NAME TABLES
 * ═══════════════════════════════════════════════════════════════════════════
 */

const char *policy_mode_name(PolicyMode mode) {
  switch (mode) {
  case POLICY_STRICT:
    return "STRICT";
  case POLICY_GUARDED:
    return "GUARDED";
  case POLICY_PERMISSIVE:
    return "PERMISSIVE";
  case POLICY_DISABLED:
    return "DISABLED";
  default:
    return "UNKNOWN";
  }
}

const char *verdict_name(ExecutionVerdict verdict) {
  switch (verdict) {
  case VERDICT_ALLOW:
    return "ALLOW";
  case VERDICT_WARN:
    return "WARN";
  case VERDICT_DENY:
    return "DENY";
  case VERDICT_QUARANTINE:
    return "QUARANTINE";
  default:
    return "UNKNOWN";
  }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY CREATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

PolicyConfig *policy_create(PolicyMode mode) {
  PolicyConfig *cfg = calloc(1, sizeof(PolicyConfig));
  if (!cfg)
    return NULL;

  cfg->mode = mode;
  cfg->rule_capacity = 16;
  cfg->rules = calloc(cfg->rule_capacity, sizeof(PolicyRule));
  cfg->enable_audit_log = false;
  cfg->audit_log_path = NULL;
  cfg->grace_period_ms = 0;
  cfg->trusted_kernels = NULL;
  cfg->trusted_count = 0;

  return cfg;
}

void policy_destroy(PolicyConfig *config) {
  if (!config)
    return;
  free(config->rules);
  free((void *)config->trusted_kernels);
  free(config);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * RULE MANAGEMENT
 * ═══════════════════════════════════════════════════════════════════════════
 */

void policy_add_rule(PolicyConfig *config, ObligationKind kind, bool required,
                     bool blocking, const char *description) {
  if (!config)
    return;

  if (config->rule_count >= config->rule_capacity) {
    size_t new_cap = config->rule_capacity * 2;
    PolicyRule *new_rules =
        realloc(config->rules, new_cap * sizeof(PolicyRule));
    if (!new_rules)
      return;
    config->rules = new_rules;
    config->rule_capacity = new_cap;
  }

  PolicyRule *r = &config->rules[config->rule_count++];
  r->kind = kind;
  r->required = required;
  r->blocking = blocking;
  r->description = description;
}

void policy_load_nova_defaults(PolicyConfig *config) {
  if (!config)
    return;

  /*
   * Nova Proof Rule:
   *
   * Kernel Valid ⇔
   *   ShapeSafe ∧ MemorySafe ∧ Deterministic ∧ NumericallyBounded
   */

  /* Zorunlu + Engelleyici (STRICT'ten bağımsız her zaman) */
  policy_add_rule(
      config, OBL_SHAPE_SAFE, true, true,
      "Shape invariants must be proven — dimension mismatches cause UB");

  policy_add_rule(config, OBL_MEMORY_SAFE, true, true,
                  "Memory safety must be proven — buffer overflows are fatal");

  policy_add_rule(
      config, OBL_NUMERICALLY_BOUND, true, true,
      "Numerical bounds must be proven — NaN/Inf corrupts computation");

  /* Zorunlu + Uyarı (GUARDED modda bloklamaz) */
  policy_add_rule(
      config, OBL_DETERMINISTIC, true, false,
      "Determinism should be proven — non-determinism affects reproducibility");

  policy_add_rule(
      config, OBL_TYPE_SAFE, true, false,
      "Type safety should be proven — incorrect casts cause corruption");

  /* İsteğe bağlı */
  policy_add_rule(config, OBL_BOUNDS_CHECK, false, false,
                  "Index bounds checking — optional but recommended");

  policy_add_rule(config, OBL_ALIASING_FREE, false, false,
                  "Aliasing freedom — relevant for parallel execution");

  /* Optimizer geçişlerinde zorunlu */
  policy_add_rule(
      config, OBL_EQUIVALENCE, true, true,
      "Optimization equivalence: f∘g ≡ h must be proven before fusion");

  /* Karar verilemez — sadece heuristik */
  policy_add_rule(config, OBL_TERMINATION, false, false,
                  "Termination — undecidable in general (Halting Problem)");
}

void policy_trust_kernel(PolicyConfig *config, const char *kernel_name) {
  if (!config || !kernel_name)
    return;

  size_t new_count = config->trusted_count + 1;
  const char **new_arr = realloc((void *)config->trusted_kernels,
                                 new_count * sizeof(const char *));
  if (!new_arr)
    return;

  new_arr[config->trusted_count] = kernel_name;
  config->trusted_kernels = new_arr;
  config->trusted_count = new_count;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CORE EVALUATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

static bool is_kernel_trusted(const PolicyConfig *config,
                              const char *kernel_name) {
  if (!config || !kernel_name)
    return false;
  for (size_t i = 0; i < config->trusted_count; i++) {
    if (config->trusted_kernels[i] &&
        strcmp(config->trusted_kernels[i], kernel_name) == 0)
      return true;
  }
  return false;
}

static bool is_obligation_satisfied(const Obligation *obl) {
  return obl->status == OBL_STATUS_PROVED || obl->status == OBL_STATUS_CACHED ||
         obl->status == OBL_STATUS_TRUSTED;
}

VerdictReport policy_evaluate(const PolicyConfig *config,
                              const ObligationSet *obligations) {
  VerdictReport report;
  memset(&report, 0, sizeof(report));

  if (!config || !obligations) {
    report.verdict = VERDICT_DENY;
    return report;
  }

  report.kernel_name = obligations->kernel_name;
  report.total_obligations = obligations->count;

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  report.timestamp_ns = ts.tv_sec * 1000000000LL + ts.tv_nsec;

  /* Mode: DISABLED → always allow */
  if (config->mode == POLICY_DISABLED) {
    report.verdict = VERDICT_ALLOW;
    report.proved_obligations = obligations->count;
    return report;
  }

  /* Trusted kernel → always allow */
  if (is_kernel_trusted(config, obligations->kernel_name)) {
    report.verdict = VERDICT_ALLOW;
    report.proved_obligations = obligations->count;
    return report;
  }

  /* Count satisfied obligations by kind */
  size_t proved = 0, failed = 0, pending = 0;
  bool has_blocking_failure = false;

  /* Temporary failure reason storage */
  size_t max_failures = obligations->count;
  const char **reasons = calloc(max_failures, sizeof(const char *));
  size_t reason_count = 0;

  for (size_t i = 0; i < obligations->count; i++) {
    const Obligation *obl = &obligations->items[i];

    if (is_obligation_satisfied(obl)) {
      proved++;
      continue;
    }

    /* Find matching rule */
    const PolicyRule *rule = NULL;
    for (size_t r = 0; r < config->rule_count; r++) {
      if (config->rules[r].kind == obl->kind) {
        rule = &config->rules[r];
        break;
      }
    }

    if (obl->status == OBL_STATUS_REFUTED) {
      failed++;
      if (rule && rule->blocking)
        has_blocking_failure = true;
      if (reasons && reason_count < max_failures) {
        reasons[reason_count++] = obl->description;
      }
    } else if (obl->status == OBL_STATUS_PENDING ||
               obl->status == OBL_STATUS_SUBMITTED) {
      pending++;
      if (rule && rule->required && rule->blocking)
        has_blocking_failure = true;
    } else {
      /* TIMEOUT, UNDECIDABLE */
      pending++;
      if (rule && rule->required && rule->blocking &&
          config->mode == POLICY_STRICT) {
        has_blocking_failure = true;
      }
    }
  }

  report.proved_obligations = proved;
  report.failed_obligations = failed;
  report.pending_obligations = pending;
  report.failure_reasons = reasons;
  report.failure_count = reason_count;

  /* Determine verdict based on mode */
  switch (config->mode) {
  case POLICY_STRICT:
    /* Her şey kanıtlanmalı */
    if (proved == obligations->count) {
      report.verdict = VERDICT_ALLOW;
    } else if (failed > 0) {
      report.verdict = VERDICT_DENY;
    } else {
      report.verdict = VERDICT_DENY; /* Pending = also deny */
    }
    break;

  case POLICY_GUARDED:
    /* Bloklayıcı olanlar zorunlu, geri kalanlar uyarı */
    if (has_blocking_failure) {
      report.verdict = VERDICT_DENY;
    } else if (pending > 0 || failed > 0) {
      report.verdict = VERDICT_WARN;
    } else {
      report.verdict = VERDICT_ALLOW;
    }
    break;

  case POLICY_PERMISSIVE:
    /* Her zaman izin ver, sadece uyar */
    if (failed > 0 || pending > 0) {
      report.verdict = VERDICT_WARN;
    } else {
      report.verdict = VERDICT_ALLOW;
    }
    break;

  default:
    report.verdict = VERDICT_ALLOW;
    break;
  }

  /* Generate verdict hash */
  uint64_t h = 0xcbf29ce484222325ULL;
  h ^= (uint64_t)report.verdict;
  h *= 0x100000001b3ULL;
  h ^= obligations->set_hash;
  h *= 0x100000001b3ULL;
  h ^= (uint64_t)report.timestamp_ns;
  h *= 0x100000001b3ULL;
  report.verdict_hash = h;

  return report;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * REPORTING
 * ═══════════════════════════════════════════════════════════════════════════
 */

void verdict_print(const VerdictReport *report) {
  if (!report)
    return;

  const char *icon;
  switch (report->verdict) {
  case VERDICT_ALLOW:
    icon = "✅";
    break;
  case VERDICT_WARN:
    icon = "⚠️";
    break;
  case VERDICT_DENY:
    icon = "❌";
    break;
  case VERDICT_QUARANTINE:
    icon = "🔒";
    break;
  default:
    icon = "❓";
    break;
  }

  fprintf(stderr,
          "\n═══════════════════════════════════════════════════════\n"
          "  %s EXECUTION VERDICT: %s\n"
          "═══════════════════════════════════════════════════════\n"
          "  Kernel:   %s\n"
          "  Verdict:  %s\n"
          "  Proved:   %zu / %zu\n"
          "  Failed:   %zu\n"
          "  Pending:  %zu\n",
          icon, verdict_name(report->verdict),
          report->kernel_name ? report->kernel_name : "(unknown)",
          verdict_name(report->verdict), report->proved_obligations,
          report->total_obligations, report->failed_obligations,
          report->pending_obligations);

  if (report->failure_count > 0) {
    fprintf(stderr, "  Failures:\n");
    for (size_t i = 0; i < report->failure_count; i++) {
      fprintf(stderr, "    ❌ %s\n",
              report->failure_reasons[i] ? report->failure_reasons[i]
                                         : "(no description)");
    }
  }

  fprintf(stderr,
          "  Hash: 0x%016llx\n"
          "═══════════════════════════════════════════════════════\n\n",
          (unsigned long long)report->verdict_hash);
}

void verdict_audit_log(const PolicyConfig *config,
                       const VerdictReport *report) {
  if (!config || !report || !config->enable_audit_log ||
      !config->audit_log_path)
    return;

  FILE *f = fopen(config->audit_log_path, "a");
  if (!f)
    return;

  fprintf(f, "[%lld] %s | %s | proved=%zu/%zu failed=%zu | hash=0x%016llx\n",
          (long long)report->timestamp_ns, verdict_name(report->verdict),
          report->kernel_name ? report->kernel_name : "?",
          report->proved_obligations, report->total_obligations,
          report->failed_obligations, (unsigned long long)report->verdict_hash);

  fclose(f);
}

int verdict_export_json(const VerdictReport *report, const char *path) {
  if (!report || !path)
    return -1;

  FILE *f = fopen(path, "w");
  if (!f)
    return -1;

  fprintf(f,
          "{\n"
          "  \"kernel\": \"%s\",\n"
          "  \"verdict\": \"%s\",\n"
          "  \"proved\": %zu,\n"
          "  \"total\": %zu,\n"
          "  \"failed\": %zu,\n"
          "  \"pending\": %zu,\n"
          "  \"hash\": \"0x%016llx\",\n"
          "  \"timestamp_ns\": %lld\n"
          "}\n",
          report->kernel_name ? report->kernel_name : "",
          verdict_name(report->verdict), report->proved_obligations,
          report->total_obligations, report->failed_obligations,
          report->pending_obligations, (unsigned long long)report->verdict_hash,
          (long long)report->timestamp_ns);

  fclose(f);
  return 0;
}

void verdict_destroy(VerdictReport *report) { report->failure_count = 0; }

/* ═══════════════════════════════════════════════════════════════════════════
 * OPTIMIZATION POLICY LOGIC
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaOptimizationConfig
nova_policy_derive_optimization(NovaOptimizationLevel opt,
                                  NovaDeterminismLevel det, bool is_verified,
                                  bool is_memory_proven) {

  NovaOptimizationConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.opt_level = opt;
  cfg.det_level = det;

  /* Baseline: No unsafe opts unless explicitly allowed */
  cfg.allow_reassoc = false;
  cfg.allow_contract = false;
  cfg.allow_reciprocal = false;
  cfg.allow_vectorize = false;
  cfg.allow_no_nans = false;
  cfg.allow_no_infs = false;

  /* STRICT Mode: Zero tolerance for non-determinism or unsafe math */
  if (opt == Z_OPT_STRICT) {
    /* Only allow safe vectorization if memory is proven */
    if (is_memory_proven && det != Z_DET_BIT_EXACT) {
      cfg.allow_vectorize = true;
    }
    return cfg;
  }

  /* VERIFIED Mode: Trust proofs */
  if (opt == Z_OPT_VERIFIED) {
    /* Safe to contract (FMA) if verified */
    if (is_verified) {
      cfg.allow_contract = true;
    }

    /* Vectorization requires memory safety proof */
    if (is_memory_proven) {
      cfg.allow_vectorize = true;
      /* If memory is proven safe, we assume bounds checks passed statically */
      cfg.allow_bounds_elision = true;
      /* Nova memory model assumes distinct tensor buffers -> noalias */
      cfg.allow_ptr_noalias = true;
      /* Nova tensors are allocated with SIMD alignment */
      cfg.assume_ptr_align = true;
    }

    /* Reassociation requires application-level determinism (or relaxed) */
    if (is_verified && det != Z_DET_BIT_EXACT) {
      cfg.allow_reassoc = true;
      cfg.allow_reciprocal = true;
    }

    /* No-NaNs/Infs assumption requires numerical bound proof (simplified as
     * verified here) */
    if (is_verified) {
      cfg.allow_no_nans = true;
      cfg.allow_no_infs = true;
    }
  }

  /* AGGRESSIVE Mode: Maximum speed */
  if (opt == Z_OPT_AGGRESSIVE) {
    /* Assume verified implies safety, push hard */
    if (is_verified) {
      cfg.allow_contract = true;
      cfg.allow_reassoc = true;
      cfg.allow_reciprocal = true;
      cfg.allow_no_nans = true;
      cfg.allow_no_infs = true;
    }

    if (is_memory_proven) {
      cfg.allow_vectorize = true;
      cfg.allow_bounds_elision = true; /* Critical for loop peeling */
      cfg.allow_ptr_noalias = true;    /* Critical for hoisting loads */
      cfg.assume_ptr_align = true;     /* Critical for AVX/NEON */
    }

    /* If relaxed determinism, allow everything even without strict proof?
       No, Nova philosophy: "Proof-Amplified".
       But aggressive mode might relax proof req slightly?
       Let's keep it proof-based but allow more transforms. */
  }

  /* Determinism Override */
  if (det == Z_DET_BIT_EXACT) {
    cfg.allow_reassoc = false;
    cfg.allow_reciprocal = false;
    /* Contract might be allowed if target FMAs are deterministic */
  }

  return cfg;
}
