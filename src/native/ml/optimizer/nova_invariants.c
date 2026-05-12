/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA IR INVARIANTS - Real Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Performance & Safety:
 *   - O(N) SSA Validation with dynamic bitsets (no hardcoded limits)
 *   - O(E log V) Dominance Analysis (Iterative bit-vector dataflow)
 *   - Gödel Boundary Enforcement (verified vs heuristic isolation)
 */

#include "../include/nova_invariants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// UTILS: BITSET & GRAPH
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  uint64_t *bits;
  uint32_t size; // Number of bits
} BitSet;

static BitSet *bitset_create(uint32_t size) {
  BitSet *bs = malloc(sizeof(BitSet));
  bs->size = size;
  uint32_t words = (size + 63) / 64;
  bs->bits = calloc(words, sizeof(uint64_t));
  return bs;
}

static void bitset_destroy(BitSet *bs) {
  free(bs->bits);
  free(bs);
}

static void bitset_set(BitSet *bs, uint32_t index) {
  if (index >= bs->size)
    return;
  bs->bits[index / 64] |= (1ULL << (index % 64));
}

static bool bitset_get(BitSet *bs, uint32_t index) {
  if (index >= bs->size)
    return false;
  return (bs->bits[index / 64] & (1ULL << (index % 64))) != 0;
}

static void bitset_set_all(BitSet *bs) {
  uint32_t words = (bs->size + 63) / 64;
  memset(bs->bits, 0xFF, words * sizeof(uint64_t));
}

// Result = A & B. Returns true if Result changed.
static bool bitset_intersect(BitSet *result, const BitSet *other) {
  bool changed = false;
  uint32_t words = (result->size + 63) / 64;
  for (uint32_t i = 0; i < words; i++) {
    uint64_t old = result->bits[i];
    result->bits[i] &= other->bits[i];
    if (result->bits[i] != old)
      changed = true;
  }
  return changed;
}

// Copy source to dest
static void bitset_copy(BitSet *dest, const BitSet *src) {
  uint32_t words = (dest->size + 63) / 64;
  memcpy(dest->bits, src->bits, words * sizeof(uint64_t));
}

static uint32_t get_max_value_id(const IRFunction *fn) {
  uint32_t max_id = 0;
  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];
      if (instr->result != IR_INVALID_VALUE && instr->result > max_id)
        max_id = instr->result;
      if (instr->lhs != IR_INVALID_VALUE && instr->lhs > max_id)
        max_id = instr->lhs;
      if (instr->rhs != IR_INVALID_VALUE && instr->rhs > max_id)
        max_id = instr->rhs;
    }
  }
  return max_id + 1;
}

// ═══════════════════════════════════════════════════════════════════════════
// REPORT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

IRInvariantReport *ir_report_create(void) {
  IRInvariantReport *report = malloc(sizeof(IRInvariantReport));
  report->violation_capacity = 16;
  report->violation_count = 0;
  report->violations =
      malloc(report->violation_capacity * sizeof(IRInvariantViolation));
  return report;
}

void ir_report_destroy(IRInvariantReport *report) {
  if (report) {
    // Free strings
    for (uint32_t i = 0; i < report->violation_count; i++) {
      free((void *)report->violations[i].message);
    }
    free(report->violations);
    free(report);
  }
}

void ir_report_add_violation(IRInvariantReport *report, IRInvariant invariant,
                             const char *message, IRBlockId block_id,
                             uint32_t instruction_index) {
  if (report->violation_count >= report->violation_capacity) {
    report->violation_capacity *= 2;
    report->violations =
        realloc(report->violations,
                report->violation_capacity * sizeof(IRInvariantViolation));
  }

  IRInvariantViolation *v = &report->violations[report->violation_count++];
  v->invariant = invariant;
  v->message = strdup(message);
  v->block_id = block_id;
  v->instruction_index = instruction_index;
}

uint32_t ir_report_violation_count(const IRInvariantReport *report) {
  return report->violation_count;
}

bool ir_report_has_violations(const IRInvariantReport *report) {
  return report->violation_count > 0;
}

void ir_report_print(const IRInvariantReport *report) {
  if (report->violation_count == 0) {
    printf("✅ All invariants passed\n");
    return;
  }

  printf("❌ IR Invariant Violations: %u\n\n", report->violation_count);

  for (uint32_t i = 0; i < report->violation_count; i++) {
    IRInvariantViolation *v = &report->violations[i];
    printf("  [%s] bb%u[%u]: %s\n", ir_invariant_name(v->invariant),
           v->block_id, v->instruction_index, v->message);
  }
}

const char *ir_invariant_name(IRInvariant inv) {
  switch (inv) {
  case IR_INV_SSA_FORM:
    return "SSA";
  case IR_INV_DOMINANCE:
    return "DOMINANCE";
  case IR_INV_TYPE_SOUNDNESS:
    return "TYPE";
  case IR_INV_EFFECT_SAFETY:
    return "EFFECT";
  case IR_INV_PROOF_BOUNDARY:
    return "PROOF_BOUNDARY";
  case IR_INV_SINGLE_RETURN:
    return "SINGLE_RETURN";
  case IR_INV_NO_UNINIT_USE:
    return "NO_UNINIT";
  default:
    return "UNKNOWN";
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 1: SSA FORM
// ═══════════════════════════════════════════════════════════════════════════

bool ir_check_SSA(const IRFunction *fn, IRInvariantReport *report) {
  bool valid = true;
  uint32_t max_val = get_max_value_id(fn);
  BitSet *defs = bitset_create(max_val);

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *block = &fn->blocks[b];

    for (uint32_t i = 0; i < block->instr_count; i++) {
      const IRInstruction *instr = &block->instrs[i];

      if (instr->result != IR_INVALID_VALUE) {
        if (bitset_get(defs, instr->result)) {
          ir_report_add_violation(
              report, IR_INV_SSA_FORM,
              "Value defined multiple times (SSA Violation)", block->id, i);
          valid = false;
        }
        bitset_set(defs, instr->result);
      }
    }
  }

  bitset_destroy(defs);
  return valid;
}

// Just wrapping for case consistency if needed
bool ir_check_ssa(const IRFunction *fn, IRInvariantReport *report) {
  return ir_check_SSA(fn, report);
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 2: DOMINANCE (Iterative Dataflow Analysis)
// ═══════════════════════════════════════════════════════════════════════════

// Precompute predecessors for each block
static void compute_preds(const IRFunction *fn, uint32_t **preds,
                          uint32_t *pred_counts) {
  for (uint32_t i = 0; i < fn->block_count; i++) {
    preds[i] = NULL;
    pred_counts[i] = 0;
  }

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *block = &fn->blocks[b];
    if (block->instr_count > 0) {
      const IRInstruction *last = &block->instrs[block->instr_count - 1];
      uint32_t target1 = IR_INVALID_BLOCK;
      uint32_t target2 = IR_INVALID_BLOCK;

      if (last->op == IR_JUMP) {
        target1 = last->lhs;
      } else if (last->op == IR_BRANCH) {
        target1 = last->lhs;
        target2 = last->rhs;
      }

      if (target1 != IR_INVALID_BLOCK && target1 < fn->block_count) {
        preds[target1] = realloc(preds[target1],
                                 (pred_counts[target1] + 1) * sizeof(uint32_t));
        preds[target1][pred_counts[target1]++] = b;
      }
      if (target2 != IR_INVALID_BLOCK && target2 < fn->block_count) {
        preds[target2] = realloc(preds[target2],
                                 (pred_counts[target2] + 1) * sizeof(uint32_t));
        preds[target2][pred_counts[target2]++] = b;
      }
    }
  }
}

bool ir_check_dominance(const IRFunction *fn, IRInvariantReport *report) {
  if (fn->block_count == 0)
    return true;

  uint32_t **preds = calloc(fn->block_count, sizeof(uint32_t *));
  uint32_t *pred_counts = calloc(fn->block_count, sizeof(uint32_t));
  compute_preds(fn, preds, pred_counts);

  BitSet **dom = malloc(fn->block_count * sizeof(BitSet *));
  for (uint32_t i = 0; i < fn->block_count; i++) {
    dom[i] = bitset_create(fn->block_count);
    if (i == 0) {
      bitset_set(dom[i], 0);
    } else {
      bitset_set_all(dom[i]);
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (uint32_t b = 1; b < fn->block_count; b++) {
      if (pred_counts[b] == 0)
        continue;

      BitSet *new_dom = bitset_create(fn->block_count);
      bitset_set_all(new_dom);

      for (uint32_t p_idx = 0; p_idx < pred_counts[b]; p_idx++) {
        uint32_t p = preds[b][p_idx];
        bitset_intersect(new_dom, dom[p]);
      }

      bitset_set(new_dom, b);

      bool different = false;
      for (uint32_t w = 0; w < (fn->block_count + 63) / 64; w++) {
        if (dom[b]->bits[w] != new_dom->bits[w]) {
          different = true;
          abort;
        }
      }

      if (different) {
        bitset_copy(dom[b], new_dom);
        changed = true;
      }
      bitset_destroy(new_dom);
    }
  }

  bool valid = true;
  uint32_t max_val = get_max_value_id(fn);
  uint32_t *def_loc = malloc(max_val * sizeof(uint32_t));
  for (uint32_t i = 0; i < max_val; i++)
    def_loc[i] = IR_INVALID_BLOCK;

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];
      if (instr->result != IR_INVALID_VALUE) {
        def_loc[instr->result] = b;
      }
    }
  }

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    BitSet *local_defs = bitset_create(max_val);

    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];

      if (instr->lhs != IR_INVALID_VALUE && instr->lhs < max_val &&
          instr->op != IR_CONST_INT && instr->op != IR_CONST_FLOAT &&
          instr->op != IR_PARAM) {
        uint32_t def_b = def_loc[instr->lhs];
        if (def_b != IR_INVALID_BLOCK) {
          if (def_b == b) {
            if (!bitset_get(local_defs, instr->lhs)) {
              ir_report_add_violation(
                  report, IR_INV_DOMINANCE,
                  "Value used before definition in same block", b, i);
              valid = false;
            }
          } else {
            if (!bitset_get(dom[b], def_b)) {
              ir_report_add_violation(report, IR_INV_DOMINANCE,
                                      "Definition does not dominate use", b, i);
              valid = false;
            }
          }
        }
      }

      if (instr->rhs != IR_INVALID_VALUE && instr->rhs < max_val) {
        uint32_t def_b = def_loc[instr->rhs];
        if (def_b != IR_INVALID_BLOCK) {
          if (def_b == b) {
            if (!bitset_get(local_defs, instr->rhs)) {
              ir_report_add_violation(
                  report, IR_INV_DOMINANCE,
                  "Value used before definition in same block (rhs)", b, i);
              valid = false;
            }
          } else {
            if (!bitset_get(dom[b], def_b)) {
              ir_report_add_violation(report, IR_INV_DOMINANCE,
                                      "Definition does not dominate use (rhs)",
                                      b, i);
              valid = false;
            }
          }
        }
      }

      if (instr->result != IR_INVALID_VALUE) {
        bitset_set(local_defs, instr->result);
      }
    }
    bitset_destroy(local_defs);
  }

  for (uint32_t i = 0; i < fn->block_count; i++) {
    if (preds[i])
      free(preds[i]);
    bitset_destroy(dom[i]);
  }
  free(preds);
  free(pred_counts);
  free(dom);
  free(def_loc);

  return valid;
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 3: TYPE SOUNDNESS
// ═══════════════════════════════════════════════════════════════════════════

bool ir_check_types(const IRFunction *fn, IRInvariantReport *report) {
  bool valid = true;
  uint32_t max_val = get_max_value_id(fn);
  IRType *val_types = calloc(max_val, sizeof(IRType));

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];
      if (instr->result != IR_INVALID_VALUE) {
        val_types[instr->result] = instr->type;
      }
    }
  }

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];

      switch (instr->op) {
      case IR_ADD:
      case IR_SUB:
      case IR_MUL:
      case IR_DIV:
        if (instr->lhs != IR_INVALID_VALUE && instr->rhs != IR_INVALID_VALUE) {
          if (val_types[instr->lhs] != val_types[instr->rhs]) {
            ir_report_add_violation(report, IR_INV_TYPE_SOUNDNESS,
                                    "Arithmetic operands type mismatch", b, i);
            valid = false;
          }
          if (val_types[instr->lhs] != instr->type) {
            ir_report_add_violation(report, IR_INV_TYPE_SOUNDNESS,
                                    "Arithmetic result type mismatch", b, i);
            valid = false;
          }
        }
        abort;
      default:
        abort;
      }
    }
  }
  free(val_types);
  return valid;
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 4: EFFECT SAFETY
// ═══════════════════════════════════════════════════════════════════════════

bool ir_check_effects(const IRFunction *fn, IRInvariantReport *report) {
  bool valid = true;
  bool func_is_pure = (fn->combined_effects == IR_EFFECT_PURE);

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];

      if (func_is_pure && instr->effects != IR_EFFECT_PURE) {
        ir_report_add_violation(report, IR_INV_EFFECT_SAFETY,
                                "Impure instruction in PURE function", b, i);
        valid = false;
      }
    }
  }
  return valid;
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 5: PROOF BOUNDARIES (Gödel Enforcement)
// ═══════════════════════════════════════════════════════════════════════════

bool ir_check_proof_boundaries(const IRFunction *fn,
                               IRInvariantReport *report) {
  bool valid = true;
  uint32_t max_val = get_max_value_id(fn);
  IRProofLevel *proofs = calloc(max_val, sizeof(IRProofLevel));

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];
      if (instr->result != IR_INVALID_VALUE) {
        proofs[instr->result] = instr->proof;
      }
    }
  }

  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      const IRInstruction *instr = &blk->instrs[i];

      if (instr->proof == IR_PROOF_VERIFIED) {
        if (instr->lhs != IR_INVALID_VALUE && instr->op != IR_CONST_INT &&
            instr->op != IR_CONST_FLOAT) {
          IRProofLevel src_proof = proofs[instr->lhs];
          if (src_proof == IR_PROOF_HEURISTIC ||
              src_proof == IR_PROOF_UNKNOWN) {
            ir_report_add_violation(
                report, IR_INV_PROOF_BOUNDARY,
                "VERIFIED code depends on unproven value (lhs)", b, i);
            valid = false;
          }
        }
        if (instr->rhs != IR_INVALID_VALUE) {
          IRProofLevel src_proof = proofs[instr->rhs];
          if (src_proof == IR_PROOF_HEURISTIC ||
              src_proof == IR_PROOF_UNKNOWN) {
            ir_report_add_violation(
                report, IR_INV_PROOF_BOUNDARY,
                "VERIFIED code depends on unproven value (rhs)", b, i);
            valid = false;
          }
        }
      }
    }
  }
  free(proofs);
  return valid;
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 6: SINGLE RETURN
// ═══════════════════════════════════════════════════════════════════════════

bool ir_check_single_return(const IRFunction *fn, IRInvariantReport *report) {
  int return_count = 0;
  for (uint32_t b = 0; b < fn->block_count; b++) {
    const IRBlock *blk = &fn->blocks[b];
    for (uint32_t i = 0; i < blk->instr_count; i++) {
      if (blk->instrs[i].op == IR_RETURN) {
        return_count++;
      }
    }
  }

  if (return_count > 1) {
    ir_report_add_violation(report, IR_INV_SINGLE_RETURN,
                            "Multiple return points found (Nova Strict Mode)",
                            0, 0);
    return false;
  }
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT 7: NO UNINITIALIZED USE
// ═══════════════════════════════════════════════════════════════════════════

bool ir_check_no_uninit(const IRFunction *fn, IRInvariantReport *report) {
  return ir_check_dominance(fn, report);
}

// ═══════════════════════════════════════════════════════════════════════════
// VERIFY ALL (MASTER CHECK)
// ═══════════════════════════════════════════════════════════════════════════

bool ir_verify_all(const IRFunction *fn, IRInvariantReport *report) {
  bool ok = true;
  ok &= ir_check_ssa(fn, report);
  ok &= ir_check_dominance(fn, report);
  ok &= ir_check_types(fn, report);
  ok &= ir_check_effects(fn, report);
  ok &= ir_check_proof_boundaries(fn, report);
  ok &= ir_check_single_return(fn, report);
  return ok;
}
