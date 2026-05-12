/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_obligation_v2_groq.c — Groq AI Optimized Constraint-Based Obligation Helpers
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_obligation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Groq AI: Obligation için delta tracking
static int groq_obligation_deltas = 0;

NovaObligation nova_obligation_begin(const char *op_name) {
  NovaObligation ob;
  memset(&ob, 0, sizeof(ob));
  ob.op_name = op_name;
  ob.all_satisfied = true;

  // Groq AI: Delta başlat
  groq_obligation_deltas = 0;

  return ob;
}

void nova_obligation_require(NovaObligation *ob, const char *name,
                               bool condition) {
  if (!ob) return;
  if (ob->num_reqs < NOVA_OBL_MAX_REQS) {
    ob->reqs[ob->num_reqs].name = name;
    ob->reqs[ob->num_reqs].satisfied = condition;
    ob->num_reqs++;

    if (!condition) {
      ob->all_satisfied = false;
      groq_obligation_deltas++; // Delta olarak işaretle
    }
  }
}

void nova_obligation_require_tensor(NovaObligation *ob, const char *name,
                                     const NovaTensor *t) {
  nova_obligation_require(ob, name, t != NULL && t->data != NULL);
  if (t) {
    groq_obligation_deltas += t->total_elements / 1000; // Tahmini delta
  }
}

void nova_obligation_report(const NovaObligation *ob) {
  printf("Groq AI: Obligation '%s' - satisfied: %d, deltas: %d\n",
         ob->op_name, ob->all_satisfied, groq_obligation_deltas);
}
