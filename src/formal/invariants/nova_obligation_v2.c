/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_obligation_v2.c — Constraint-Based Obligation Helpers
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_obligation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * NovaObligation — Single op obligation
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_obligation_begin(const char *op_name) {
  NovaObligation ob;
  memset(&ob, 0, sizeof(ob));
  ob.op_name = op_name;
  ob.all_satisfied = true;
  return ob;
}

void nova_obligation_require(NovaObligation *ob, const char *name,
                               bool condition) {
  if (!ob)
    return;
  if (ob->num_reqs < NOVA_OBL_MAX_REQS) {
    ob->reqs[ob->num_reqs].name = name;
    ob->reqs[ob->num_reqs].satisfied = condition;
    ob->num_reqs++;
  }
  if (!condition) {
    ob->all_satisfied = false;
  }
}

void nova_obligation_require_tensor(NovaObligation *ob, const char *name,
                                      const void *tensor) {
  nova_obligation_require(ob, name, tensor != NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * NovaObligationList — Collection of obligations
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligationList nova_obligation_list_create(void) {
  NovaObligationList list;
  memset(&list, 0, sizeof(list));
  list.capacity = 64;
  list.items = calloc(list.capacity, sizeof(NovaObligation));
  return list;
}

void nova_obligation_list_push(NovaObligationList *list,
                                 NovaObligation ob) {
  if (!list)
    return;
  if (list->count >= list->capacity) {
    list->capacity = list->capacity ? list->capacity * 2 : 64;
    list->items =
        realloc(list->items, list->capacity * sizeof(NovaObligation));
  }
  list->items[list->count++] = ob;
}

void nova_obligation_list_destroy(NovaObligationList *list) {
  if (!list)
    return;
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

bool nova_obligation_list_all_satisfied(const NovaObligationList *list) {
  if (!list)
    return true;
  for (uint32_t i = 0; i < list->count; i++) {
    if (!list->items[i].all_satisfied)
      return false;
  }
  return true;
}

void nova_obligation_list_print(const NovaObligationList *list) {
  if (!list)
    return;

  uint32_t total_reqs = 0;
  uint32_t failed_reqs = 0;

  for (uint32_t i = 0; i < list->count; i++) {
    const NovaObligation *ob = &list->items[i];
    const char *status = ob->all_satisfied ? "✅" : "⛔";
    fprintf(stderr, "  %s [%s] %u checks", status,
            ob->op_name ? ob->op_name : "?", ob->num_reqs);

    if (!ob->all_satisfied) {
      fprintf(stderr, " — FAILURES:");
      for (uint32_t j = 0; j < ob->num_reqs; j++) {
        if (!ob->reqs[j].satisfied) {
          fprintf(stderr, " %s", ob->reqs[j].name);
          failed_reqs++;
        }
      }
    }
    fprintf(stderr, "\n");
    total_reqs += ob->num_reqs;
  }

  fprintf(stderr,
          "  ───────────────────────────────────────────\n"
          "  %u obligations, %u total checks, %u failures\n",
          list->count, total_reqs, failed_reqs);
}
