/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_obligation.c — Proof Obligation Engine Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_obligation.h"
#include "nova_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ─── Globals ────────────────────────────────────────────────────────────── */

static uint64_t g_next_obligation_id = 1;

/* ─── Kind name table ────────────────────────────────────────────────────── */

const char *obligation_kind_name(ObligationKind kind) {
  switch (kind) {
  case OBL_SHAPE_SAFE:
    return "ShapeSafe";
  case OBL_MEMORY_SAFE:
    return "MemorySafe";
  case OBL_DETERMINISTIC:
    return "Deterministic";
  case OBL_NUMERICALLY_BOUND:
    return "NumericallyBounded";
  case OBL_TYPE_SAFE:
    return "TypeSafe";
  case OBL_BOUNDS_CHECK:
    return "BoundsCheck";
  case OBL_ALIASING_FREE:
    return "AliasingFree";
  case OBL_EQUIVALENCE:
    return "Equivalence";
  case OBL_TERMINATION:
    return "Termination";
  case OBL_RESOURCE_BOUND:
    return "ResourceBound";
  default:
    return "Unknown";
  }
}

const char *obligation_status_name(ObligationStatus status) {
  switch (status) {
  case OBL_STATUS_PENDING:
    return "PENDING";
  case OBL_STATUS_SUBMITTED:
    return "SUBMITTED";
  case OBL_STATUS_PROVED:
    return "PROVED";
  case OBL_STATUS_REFUTED:
    return "REFUTED";
  case OBL_STATUS_TIMEOUT:
    return "TIMEOUT";
  case OBL_STATUS_CACHED:
    return "CACHED";
  case OBL_STATUS_TRUSTED:
    return "TRUSTED";
  case OBL_STATUS_UNDECIDABLE:
    return "UNDECIDABLE";
  default:
    return "UNKNOWN";
  }
}

/* ─── Decidability check ─────────────────────────────────────────────────── */

bool obligation_is_decidable(ObligationKind kind) {
  switch (kind) {
  case OBL_SHAPE_SAFE:
  case OBL_MEMORY_SAFE:
  case OBL_NUMERICALLY_BOUND:
  case OBL_TYPE_SAFE:
  case OBL_BOUNDS_CHECK:
  case OBL_ALIASING_FREE:
    return true;

  /* Gödel sınırı: bu özellikler genel durumda karar verilemez */
  case OBL_TERMINATION:
  case OBL_EQUIVALENCE: /* Genel eşdeğerlik karar verilemez */
    return false;

  case OBL_DETERMINISTIC:
  case OBL_RESOURCE_BOUND:
    return true; /* Sınırlı bağlamda karar verilebilir */

  default:
    return false;
  }
}

/* ─── ObligationSet ──────────────────────────────────────────────────────── */

#define INITIAL_CAPACITY 16

ObligationSet *obligation_set_create(const char *kernel_name) {
  ObligationSet *set = calloc(1, sizeof(ObligationSet));
  if (!set)
    return NULL;

  set->items = calloc(INITIAL_CAPACITY, sizeof(Obligation));
  if (!set->items) {
    free(set);
    return NULL;
  }

  set->capacity = INITIAL_CAPACITY;
  set->count = 0;
  set->kernel_name = kernel_name;
  set->all_proved = false;
  set->proved_count = 0;
  set->refuted_count = 0;
  set->pending_count = 0;
  set->total_solve_ms = 0.0;
  set->set_hash = 0;

  return set;
}

Obligation *obligation_set_add(ObligationSet *set, ObligationKind kind,
                               const char *description,
                               const char *smt2_formula) {
  if (!set)
    return NULL;

  /* Grow if needed */
  if (set->count >= set->capacity) {
    size_t new_cap = set->capacity * 2;
    Obligation *new_items = realloc(set->items, new_cap * sizeof(Obligation));
    if (!new_items)
      return NULL;
    set->items = new_items;
    set->capacity = new_cap;
  }

  Obligation *obl = &set->items[set->count];
  memset(obl, 0, sizeof(Obligation));

  obl->id = g_next_obligation_id++;
  obl->kind = kind;
  obl->status = OBL_STATUS_PENDING;
  obl->source_kernel = set->kernel_name;
  obl->description = description;
  obl->smt2_formula = smt2_formula;
  obl->receipt = NULL;
  obl->is_decidable = obligation_is_decidable(kind);
  obl->timeout_ms = 5000.0; /* Default 5 seconds */

  set->count++;
  set->pending_count++;

  /* Update set hash (simple FNV-1a) */
  uint64_t h = set->set_hash ^ 0xcbf29ce484222325ULL;
  h ^= obl->id;
  h *= 0x100000001b3ULL;
  h ^= (uint64_t)kind;
  h *= 0x100000001b3ULL;
  set->set_hash = h;

  return obl;
}

void obligation_set_destroy(ObligationSet *set) {
  if (!set)
    return;

  for (size_t i = 0; i < set->count; i++) {
    if (set->items[i].receipt) {
      free(set->items[i].receipt);
    }
  }
  free(set->items);
  free(set);
}

bool obligation_set_is_discharged(const ObligationSet *set) {
  if (!set)
    return false;

  for (size_t i = 0; i < set->count; i++) {
    ObligationStatus s = set->items[i].status;
    if (s != OBL_STATUS_PROVED && s != OBL_STATUS_CACHED &&
        s != OBL_STATUS_TRUSTED) {
      return false;
    }
  }
  return true;
}

/* ─── Summary ────────────────────────────────────────────────────────────── */

void obligation_set_print_summary(const ObligationSet *set) {
  if (!set)
    return;

  fprintf(stderr,
          "\n═══════════════════════════════════════════════════════\n"
          "  PROOF OBLIGATIONS: %s\n"
          "═══════════════════════════════════════════════════════\n",
          set->kernel_name ? set->kernel_name : "(unnamed)");

  size_t proved = 0, refuted = 0, pending = 0, other = 0;

  for (size_t i = 0; i < set->count; i++) {
    const Obligation *o = &set->items[i];
    const char *icon;
    switch (o->status) {
    case OBL_STATUS_PROVED:
      icon = "✅";
      proved++;
      break;
    case OBL_STATUS_CACHED:
      icon = "⚡";
      proved++;
      break;
    case OBL_STATUS_TRUSTED:
      icon = "🔒";
      proved++;
      break;
    case OBL_STATUS_REFUTED:
      icon = "❌";
      refuted++;
      break;
    case OBL_STATUS_PENDING:
      icon = "⏳";
      pending++;
      break;
    default:
      icon = "⚠️";
      other++;
      break;
    }

    fprintf(stderr, "  %s [%s] %s\n", icon, obligation_kind_name(o->kind),
            o->description ? o->description : "");

    if (o->receipt && o->receipt->counterexample) {
      fprintf(stderr, "     ↳ Counterexample: %s\n",
              o->receipt->counterexample);
    }
  }

  fprintf(stderr,
          "───────────────────────────────────────────────────────\n"
          "  Total: %zu | Proved: %zu | Refuted: %zu | Pending: %zu\n",
          set->count, proved, refuted, pending);

  if (set->total_solve_ms > 0) {
    fprintf(stderr, "  Solve time: %.2f ms\n", set->total_solve_ms);
  }

  bool discharged = obligation_set_is_discharged(set);
  fprintf(stderr,
          "  Status: %s\n"
          "═══════════════════════════════════════════════════════\n\n",
          discharged ? "✅ ALL DISCHARGED — Execution PERMITTED"
                     : "❌ UNDISCHARGED — Execution BLOCKED");
}

/* ─── JSON Export ────────────────────────────────────────────────────────── */

int obligation_set_export_json(const ObligationSet *set,
                               const char *output_path) {
  if (!set || !output_path)
    return -1;

  FILE *f = fopen(output_path, "w");
  if (!f)
    return -1;

  fprintf(f, "{\n");
  fprintf(f, "  \"kernel\": \"%s\",\n",
          set->kernel_name ? set->kernel_name : "");
  fprintf(f, "  \"set_hash\": \"%llu\",\n", (unsigned long long)set->set_hash);
  fprintf(f, "  \"discharged\": %s,\n",
          obligation_set_is_discharged(set) ? "true" : "false");
  fprintf(f, "  \"total_solve_ms\": %.3f,\n", set->total_solve_ms);
  fprintf(f, "  \"obligations\": [\n");

  for (size_t i = 0; i < set->count; i++) {
    const Obligation *o = &set->items[i];
    fprintf(f, "    {\n");
    fprintf(f, "      \"id\": %llu,\n", (unsigned long long)o->id);
    fprintf(f, "      \"kind\": \"%s\",\n", obligation_kind_name(o->kind));
    fprintf(f, "      \"status\": \"%s\",\n",
            obligation_status_name(o->status));
    fprintf(f, "      \"description\": \"%s\",\n",
            o->description ? o->description : "");
    fprintf(f, "      \"decidable\": %s\n", o->is_decidable ? "true" : "false");
    fprintf(f, "    }%s\n", (i < set->count - 1) ? "," : "");
  }

  fprintf(f, "  ]\n}\n");
  fclose(f);
  return 0;
}
