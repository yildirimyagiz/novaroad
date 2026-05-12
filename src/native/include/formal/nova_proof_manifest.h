/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_proof_manifest.h - Build-Time Formal Verification Manifest
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * This file is AUTO-GENERATED during the build process by running:
 *   - Isabelle/HOL theorem prover
 *   - KLEE symbolic execution
 *   - CVC5 SMT solver
 *
 * DO NOT call verification tools at runtime - all proofs are checked at
 * compile time and embedded as a signed manifest.
 *
 * Inspired by: seL4, CompCert, Everest (verified crypto)
 */

#ifndef NOVA_PROOF_MANIFEST_H
#define NOVA_PROOF_MANIFEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Single proof entry in the manifest
 */
typedef struct {
  const char *proof_name;          // e.g., "tensor_invariants"
  const char *hash_sha256;         // SHA256 of proof artifact
  const char *tool;                // "isabelle", "klee", "cvc5"
  bool verified_at_build_time;     // Did proof pass?
  uint64_t verification_timestamp; // Unix timestamp
  const char *theorem_statement;   // Human-readable theorem (optional)
} NovaProofEntry;

/**
 * Complete proof manifest (populated at build time)
 */
typedef struct {
  NovaProofEntry *entries;
  size_t count;
  const char *build_git_commit;   // Git SHA of verified code
  const char *build_timestamp;    // Build date/time
  const char *compiler_version;   // GCC/Clang version
  uint8_t manifest_signature[64]; // Optional: cryptographic signature
} NovaProofManifest;

/**
 * Global manifest instance (defined in auto-generated .c file)
 */
extern const NovaProofManifest NOVA_PROOF_MANIFEST;

/**
 * Runtime validation (O(1) hash lookup)
 */
bool nova_proof_validate_manifest(void);
bool nova_proof_check(const char *proof_name);

/**
 * Query proof status
 */
const NovaProofEntry *nova_proof_find(const char *proof_name);
void nova_proof_print_manifest(void);

/**
 * Build-time code generation (called by CMake)
 */
#ifdef NOVA_BUILD_TOOLS
void nova_proof_manifest_generate(const char *output_path);
#endif

#endif // NOVA_PROOF_MANIFEST_H
