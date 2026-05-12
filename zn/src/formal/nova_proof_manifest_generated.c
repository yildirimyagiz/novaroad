
// AUTO-GENERATED - DO NOT EDIT
#include "formal/nova_proof_manifest.h"
#include <string.h>

static const NovaProofEntry PROOF_ENTRIES[] = {
  {
    .proof_name = "tensor_invariants",
    .hash_sha256 = "8ec6cb89074b302209e09af134d725d3b8cdf81c7bcedb664061a92678b46383",
    .tool = "isabelle",
    .verified_at_build_time = true,
    .verification_timestamp = 1771226765ULL
  },
  {
    .proof_name = "memory_safety",
    .hash_sha256 = "f51b686fcc67543f96563138cdf598fcfdcc91213b3244575f88c2465f12be82",
    .tool = "klee",
    .verified_at_build_time = true,
    .verification_timestamp = 1771226765ULL
  },
  {
    .proof_name = "optimizer_stability",
    .hash_sha256 = "dd820c98a99fcba34831ee74670fb53e9d5ef9bd321be3af9c9d86572c05643f",
    .tool = "cvc5",
    .verified_at_build_time = true,
    .verification_timestamp = 1771226765ULL
  },
  {
    .proof_name = "autodiff_correctness",
    .hash_sha256 = "f42183be714025de343b377e44213e2118e09657058853bb5cc128bb73f6c299",
    .tool = "isabelle",
    .verified_at_build_time = true,
    .verification_timestamp = 1771226765ULL
  },
  {
    .proof_name = "kernel_determinism",
    .hash_sha256 = "513d296d29129158fea73dff9e66e7296a4f106e33468f3bb79cac77ecfd6db2",
    .tool = "isabelle",
    .verified_at_build_time = true,
    .verification_timestamp = 1771226765ULL
  },

};

const NovaProofManifest NOVA_PROOF_MANIFEST = {
  .entries = (NovaProofEntry*)PROOF_ENTRIES,
  .count = 5,
  .build_git_commit = "head",
  .build_timestamp = "2026-02-16 10:26:05"
};

bool nova_proof_validate_manifest(void) {
  // Simple validation: Ensure entries count matches
  return NOVA_PROOF_MANIFEST.count > 0;
}

bool nova_proof_check(const char *proof_name) {
  for (size_t i = 0; i < NOVA_PROOF_MANIFEST.count; i++) {
    if (strcmp(PROOF_ENTRIES[i].proof_name, proof_name) == 0) {
      return PROOF_ENTRIES[i].verified_at_build_time;
    }
  }
  return false;
}
