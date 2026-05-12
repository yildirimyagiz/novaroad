/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA POLICY — Execution Permission Engine
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * "Program doğru mu?" yerine "Program çalışmaya izinli mi?"
 *
 * Policy engine, kernel çalıştırma iznini proof receipt'lere göre verir.
 * Bu ayrım bilgisayar bilimi tarihinde radikal bir sıçramadır.
 *
 * Modlar:
 *   STRICT:   Tüm proof obligation'lar kanıtlanmadan execution yok
 *   GUARDED:  Kritik obligation'lar zorunlu, geri kalanı uyarı
 *   PERMISSIVE: Uyarı ver ama çalıştır (geliştirme modu)
 *   DISABLED: Kontrol yok (legacy uyumluluk)
 */

#ifndef NOVA_POLICY_H
#define NOVA_POLICY_H

#include "nova_obligation.h"
#include <stdbool.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY MODE
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  POLICY_STRICT,     /* Proof-driven execution: kanıtsız çalışmaz    */
  POLICY_GUARDED,    /* Kritik borçlar zorunlu, diğerleri uyarı       */
  POLICY_PERMISSIVE, /* Hep çalışır, uyarı loglar                    */
  POLICY_DISABLED,   /* Sıfır kontrol — legacy mod                   */
} PolicyMode;

/* ═══════════════════════════════════════════════════════════════════════════
 * EXECUTION VERDICT — Çalışma izni kararı
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  VERDICT_ALLOW,      /* ✅ Tüm kanıtlar tamam — çalışabilir          */
  VERDICT_WARN,       /* ⚠️ Eksik kanıt var ama izin verildi           */
  VERDICT_DENY,       /* ❌ Kanıt borcu ödenmedi — çalışma engellendi  */
  VERDICT_QUARANTINE, /* 🔒 Çalışır ama sonuç "güvenilmez" etiketli   */
} ExecutionVerdict;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY RULE — Hangi obligation türü ne gerektirir?
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  ObligationKind kind;
  bool required; /* Bu obligation türü zorunlu mu?        */
  bool blocking; /* Eksikse çalışmayı engeller mi?        */
  const char *description;
} PolicyRule;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY CONFIGURATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  PolicyMode mode;
  PolicyRule *rules;
  size_t rule_count;
  size_t rule_capacity;

  /* Audit trail */
  bool enable_audit_log;
  const char *audit_log_path;

  /* Grace period: N saniye boyunca pending obligation'lar izin alır */
  double grace_period_ms;

  /* Trusted kernels: İsimle tanımlı güvenilir kernel'ler */
  const char **trusted_kernels;
  size_t trusted_count;
} PolicyConfig;

/* ═══════════════════════════════════════════════════════════════════════════
 * VERDICT REPORT — Detaylı karar raporu
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  ExecutionVerdict verdict;
  const char *kernel_name;
  size_t total_obligations;
  size_t proved_obligations;
  size_t failed_obligations;
  size_t pending_obligations;
  const char **failure_reasons;
  size_t failure_count;
  int64_t timestamp_ns;
  uint64_t verdict_hash; /* Tamper-proof audit hash          */
} VerdictReport;

/* ═══════════════════════════════════════════════════════════════════════════
 * API
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Policy engine oluştur
 */
PolicyConfig *policy_create(PolicyMode mode);

/**
 * Policy engine'i yık
 */
void policy_destroy(PolicyConfig *config);

/**
 * Varsayılan Nova kurallarını yükle:
 * - SHAPE_SAFE: zorunlu, engelleyici
 * - MEMORY_SAFE: zorunlu, engelleyici
 * - NUMERICALLY_BOUND: zorunlu, engelleyici
 * - DETERMINISTIC: zorunlu, uyarı
 * - TYPE_SAFE: isteğe bağlı
 * - EQUIVALENCE: zorunlu (optimizer geçişlerinde)
 */
void policy_load_nova_defaults(PolicyConfig *config);

/**
 * Özel kural ekle
 */
void policy_add_rule(PolicyConfig *config, ObligationKind kind, bool required,
                     bool blocking, const char *description);

/**
 * Bir kernel'in trust listesine ekle
 * Güvenilir kernel'ler obligation kontrolünü bypass eder
 */
void policy_trust_kernel(PolicyConfig *config, const char *kernel_name);

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * ANA KARAR FONKSİYONU
 *
 * "Bu kernel çalışmaya izinli mi?"
 *
 * ObligationSet + PolicyConfig → VerdictReport
 * ═══════════════════════════════════════════════════════════════════════════
 */
VerdictReport policy_evaluate(const PolicyConfig *config,
                              const ObligationSet *obligations);

/**
 * Verdict raporu yazdır
 */
void verdict_print(const VerdictReport *report);

/**
 * Verdict raporunu JSON olarak dışa aktar
 */
int verdict_export_json(const VerdictReport *report, const char *path);

/**
 * Verdict raporunu audit log'a yaz
 */
void verdict_audit_log(const PolicyConfig *config, const VerdictReport *report);

/**
 * Verdict raporunu yık
 */
void verdict_destroy(VerdictReport *report);

/**
 * Policy modunun adını döndür
 */
const char *policy_mode_name(PolicyMode mode);

/**
 * Verdict türünün adını döndür
 */
const char *verdict_name(ExecutionVerdict verdict);

/* ═══════════════════════════════════════════════════════════════════════════
 * MACRO — Inline policy check
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Kullanım:
 *   NOVA_REQUIRE_PERMISSION(policy, obligations, "matmul_f32");
 */

#define NOVA_REQUIRE_PERMISSION(policy, obligations, kname)                  \
  do {                                                                         \
    VerdictReport _vr = policy_evaluate((policy), (obligations));              \
    if (_vr.verdict == VERDICT_DENY) {                                         \
      fprintf(stderr,                                                          \
              "❌ [Policy] EXECUTION DENIED: %s\n"                             \
              "   Proved: %zu/%zu obligations\n"                               \
              "   Failed: %zu\n",                                              \
              (kname), _vr.proved_obligations, _vr.total_obligations,          \
              _vr.failed_obligations);                                         \
      verdict_audit_log((policy), &_vr);                                       \
      verdict_destroy(&_vr);                                                   \
      return;                                                                  \
    }                                                                          \
    if (_vr.verdict == VERDICT_WARN) {                                         \
      fprintf(stderr, "⚠️  [Policy] WARNING: %s — %zu pending obligations\n",   \
              (kname), _vr.pending_obligations);                               \
    }                                                                          \
    verdict_audit_log((policy), &_vr);                                         \
    verdict_destroy(&_vr);                                                     \
  } while (0)

/* ═══════════════════════════════════════════════════════════════════════════
 * PROOF-DRIVEN OPTIMIZATION POLICY
 * ═══════════════════════════════════════════════════════════════════════════
 */

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

/* Helper to derive config from proofs and levels */
NovaOptimizationConfig
nova_policy_derive_optimization(NovaOptimizationLevel opt,
                                  NovaDeterminismLevel det, bool is_verified,
                                  bool is_memory_proven);

#endif /* NOVA_POLICY_H */
