/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA PROOF OBLIGATION SYSTEM
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * "execution = proof obligation"
 *
 * Her kernel çalıştırıldığında bir KANIT BORCU üretir.
 * Bu borç ödenmeden (solver tarafından doğrulanmadan)
 * sonuç "geçerli" sayılmaz.
 *
 * Gödel'in aritmetizasyon fikri:
 *   İfade → Sembol → Aksiyom → Çıkarım → İspat
 *
 * Nova'te:
 *   Kernel → Obligation → SymbolicIR → Solver → ProofReceipt
 */

#ifndef NOVA_OBLIGATION_H
#define NOVA_OBLIGATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * OBLIGATION CATEGORIES
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Kernel Valid ⇔
 *   ShapeSafe ∧ MemorySafe ∧ Deterministic ∧ NumericallyBounded
 */

typedef enum {
  OBL_SHAPE_SAFE = 0x0001,        /* Boyut uyumu (shape invariants)       */
  OBL_MEMORY_SAFE = 0x0002,       /* Bellek taşması yok                   */
  OBL_DETERMINISTIC = 0x0004,     /* Aynı input → aynı output             */
  OBL_NUMERICALLY_BOUND = 0x0008, /* NaN / Inf kontrolü                   */
  OBL_TYPE_SAFE = 0x0010,         /* Tip dönüşüm güvenliği                */
  OBL_BOUNDS_CHECK = 0x0020,      /* Index sınır kontrolü                 */
  OBL_ALIASING_FREE = 0x0040,     /* Bellek aliasing yok                  */
  OBL_EQUIVALENCE = 0x0080,       /* Optimizasyon eşdeğerliği: f∘g ≡ h   */
  OBL_TERMINATION = 0x0100,       /* Sonsuz döngü yok (undecidable!)      */
  OBL_RESOURCE_BOUND = 0x0200,    /* Bellek / zaman sınırı                */

  /* Bileşik maskeler */
  OBL_KERNEL_MINIMUM = OBL_SHAPE_SAFE | OBL_MEMORY_SAFE | OBL_NUMERICALLY_BOUND,
  OBL_KERNEL_FULL = OBL_SHAPE_SAFE | OBL_MEMORY_SAFE | OBL_DETERMINISTIC |
                    OBL_NUMERICALLY_BOUND | OBL_TYPE_SAFE | OBL_BOUNDS_CHECK,
  OBL_OPTIMIZER_PASS = OBL_EQUIVALENCE | OBL_SHAPE_SAFE | OBL_DETERMINISTIC,
} ObligationKind;

/* ═══════════════════════════════════════════════════════════════════════════
 * OBLIGATION STATUS
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  OBL_STATUS_PENDING = 0,     /* Henüz solver'a gönderilmedi             */
  OBL_STATUS_SUBMITTED = 1,   /* Solver'da çözülüyor                     */
  OBL_STATUS_PROVED = 2,      /* İspatlandı ✅                            */
  OBL_STATUS_REFUTED = 3,     /* Çürütüldü — karşı-örnek bulundu ❌       */
  OBL_STATUS_TIMEOUT = 4,     /* Solver zaman aşımı — Gödel sınırı ⚠️    */
  OBL_STATUS_CACHED = 5,      /* Önbellekten doğrulandı ⚡                */
  OBL_STATUS_TRUSTED = 6,     /* Manuel olarak güvenildi (axiom)          */
  OBL_STATUS_UNDECIDABLE = 7, /* Karar verilemez (halting problem vb.)    */
} ObligationStatus;

/* ═══════════════════════════════════════════════════════════════════════════
 * PROOF RECEIPT — Bir obligation'ın ispat sertifikası
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  uint64_t receipt_id;
  uint64_t obligation_id;
  ObligationStatus status;
  const char *solver_name; /* "cvc5", "z3", "fallback"            */
  double solve_time_ms;
  uint64_t proof_hash;        /* SHA-256(obligation + result)         */
  const char *counterexample; /* NULL if proved, model if refuted     */
  int64_t timestamp_ns;       /* Nanosecond precision                 */
} ProofReceipt;

/* ═══════════════════════════════════════════════════════════════════════════
 * OBLIGATION — Tek bir kanıt borcu
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  uint64_t id;
  ObligationKind kind;
  ObligationStatus status;
  const char *source_kernel; /* "matmul_f32_avx512"               */
  const char *source_file;   /* __FILE__                          */
  int source_line;           /* __LINE__                          */
  const char *description;   /* İnsan okunabilir açıklama          */
  const char *smt2_formula;  /* SMT-LIB2 formatında formül        */
  ProofReceipt *receipt;     /* NULL until solved                  */
  bool is_decidable;         /* Gödel sınırı bilgisi              */
  double timeout_ms;         /* Solver timeout                    */
} Obligation;

/* ═══════════════════════════════════════════════════════════════════════════
 * OBLIGATION SET — Bir kernel'in tüm borçları
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  Obligation *items;
  size_t count;
  size_t capacity;
  const char *kernel_name;
  uint64_t set_hash; /* Tüm borçların birleşik hash'i      */
  bool all_proved;   /* Tüm borçlar ispatlandı mı?         */
  size_t proved_count;
  size_t refuted_count;
  size_t pending_count;
  double total_solve_ms;
} ObligationSet;

/* ═══════════════════════════════════════════════════════════════════════════
 * API
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Boş obligation set oluştur
 */
ObligationSet *obligation_set_create(const char *kernel_name);

/**
 * Obligation ekle
 */
Obligation *obligation_set_add(ObligationSet *set, ObligationKind kind,
                               const char *description,
                               const char *smt2_formula);

/**
 * Obligation set'i yık
 */
void obligation_set_destroy(ObligationSet *set);

/**
 * Tüm borçlar ispatlandı mı kontrol et
 */
bool obligation_set_is_discharged(const ObligationSet *set);

/**
 * Obligation'ın karar verilebilir olup olmadığını belirle
 */
bool obligation_is_decidable(ObligationKind kind);

/**
 * Obligation türünün adını döndür
 */
const char *obligation_kind_name(ObligationKind kind);

/**
 * Obligation durumunun adını döndür
 */
const char *obligation_status_name(ObligationStatus status);

/**
 * Obligation set özeti yazdır
 */
void obligation_set_print_summary(const ObligationSet *set);

/**
 * Obligation set'i JSON olarak dışa aktar
 */
int obligation_set_export_json(const ObligationSet *set,
                               const char *output_path);

/* ═══════════════════════════════════════════════════════════════════════════
 * MACROS — Kod içinden kanıt borcu üret
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * EMIT_OBLIGATION(set, OBL_SHAPE_SAFE,
 *     "matmul: M×K × K×N → M×N",
 *     "(assert (= (dim out 0) (dim a 0)))");
 */

#define EMIT_OBLIGATION(set, kind, desc, smt2)                                 \
  obligation_set_add((set), (kind), (desc), (smt2))

#define EMIT_SHAPE_OBLIGATION(set, desc, smt2)                                 \
  EMIT_OBLIGATION((set), OBL_SHAPE_SAFE, (desc), (smt2))

#define EMIT_MEMORY_OBLIGATION(set, desc, smt2)                                \
  EMIT_OBLIGATION((set), OBL_MEMORY_SAFE, (desc), (smt2))

#define EMIT_DETERMINISM_OBLIGATION(set, desc, smt2)                           \
  EMIT_OBLIGATION((set), OBL_DETERMINISTIC, (desc), (smt2))

#define EMIT_NUMERIC_OBLIGATION(set, desc, smt2)                               \
  EMIT_OBLIGATION((set), OBL_NUMERICALLY_BOUND, (desc), (smt2))

#define EMIT_EQUIVALENCE_OBLIGATION(set, desc, smt2)                           \
  EMIT_OBLIGATION((set), OBL_EQUIVALENCE, (desc), (smt2))

/* ═══════════════════════════════════════════════════════════════════════════
 * NOVA OBLIGATION V2 — Constraint-Based Contract System
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Lightweight, value-type obligation used by kernel contracts.
 * Each requirement is a named boolean check evaluated eagerly.
 */

#define NOVA_OBL_MAX_REQS 32

typedef struct {
  const char *name;
  bool satisfied;
} NovaObligationReq;

typedef struct NovaObligation {
  const char *op_name;
  NovaObligationReq reqs[NOVA_OBL_MAX_REQS];
  uint32_t num_reqs;
  bool all_satisfied;
} NovaObligation;

typedef struct {
  NovaObligation *items;
  uint32_t count;
  uint32_t capacity;
} NovaObligationList;

/* V2 API */
NovaObligation nova_obligation_begin(const char *op_name);
void nova_obligation_require(NovaObligation *ob, const char *name,
                               bool condition);
void nova_obligation_require_tensor(NovaObligation *ob, const char *name,
                                      const void *tensor);

NovaObligationList nova_obligation_list_create(void);
void nova_obligation_list_push(NovaObligationList *list,
                                 NovaObligation ob);
void nova_obligation_list_destroy(NovaObligationList *list);
bool nova_obligation_list_all_satisfied(const NovaObligationList *list);
void nova_obligation_list_print(const NovaObligationList *list);

#endif /* NOVA_OBLIGATION_H */
