#ifndef NOVA_ISABELLE_BRIDGE_H
#define NOVA_ISABELLE_BRIDGE_H

#include <stdbool.h>

/**
 * Isabelle/HOL integration for Nova.
 * Handles theory export and proof artifact verification.
 */

typedef struct {
  char *theory_name;
  char *proof_hash;
  bool is_verified;
  void *signature;
} NovaIsabelleArtifact;

// Export Nova specification to Isabelle .thy file
int nova_isabelle_export_theory(const char *spec_name, const char *content,
                                  const char *path);

// Load and verify an Isabelle proof artifact
NovaIsabelleArtifact *nova_isabelle_load_artifact(const char *path);

// Verify if a specific invariant is proved by an artifact
bool nova_isabelle_verify_invariant(NovaIsabelleArtifact *artifact,
                                      const char *invariant_name);

#endif // NOVA_ISABELLE_BRIDGE_H
