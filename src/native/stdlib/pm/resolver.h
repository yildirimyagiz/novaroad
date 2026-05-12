/**
 * Nova Package Manager - Dependency Resolver
 */

#ifndef NOVA_PM_RESOLVER_H
#define NOVA_PM_RESOLVER_H

#include "registry.h"
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// RESOLUTION TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *name;
  char *requested_version; // e.g., "^1.0.0", ">=2.0", "1.2.3"
  char *resolved_version;  // e.g., "1.2.5"
} ResolvedDep;

typedef struct {
  ResolvedDep *deps;
  size_t count;
  bool success;
  char *error;
} ResolutionResult;

// ═══════════════════════════════════════════════════════════════════════════
// VERSION CONSTRAINTS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  VERSION_EXACT, // "1.2.3"
  VERSION_CARET, // "^1.2.3" - compatible updates
  VERSION_TILDE, // "~1.2.3" - patch updates only
  VERSION_GTE,   // ">=1.2.3"
  VERSION_LTE,   // "<=1.2.3"
  VERSION_GT,    // ">1.2.3"
  VERSION_LT,    // "<1.2.3"
  VERSION_RANGE, // ">=1.0.0 <2.0.0"
  VERSION_ANY    // "*" or "latest"
} VersionConstraintType;

typedef struct {
  VersionConstraintType type;
  int major;
  int minor;
  int patch;
  int major2; // For ranges
  int minor2;
  int patch2;
} VersionConstraint;

// ═══════════════════════════════════════════════════════════════════════════
// RESOLVER API
// ═══════════════════════════════════════════════════════════════════════════

// Parse version string (e.g., "1.2.3")
bool parse_version(const char *str, int *major, int *minor, int *patch);

// Parse version constraint (e.g., "^1.2.3")
VersionConstraint *parse_constraint(const char *str);

// Check if version satisfies constraint
bool version_satisfies(const char *version, VersionConstraint *constraint);

// Compare versions (-1, 0, 1)
int version_compare(const char *v1, const char *v2);

// Resolve all dependencies for a package
ResolutionResult *resolve_deps(const char *name, const char *version);

// Resolve from project manifest (nova.json)
ResolutionResult *resolve_manifest(const char *manifest_path);

// Get install order (topological sort)
char **get_install_order(ResolutionResult *result, size_t *count);

// ═══════════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

void constraint_free(VersionConstraint *c);
void resolution_free(ResolutionResult *r);

#endif // NOVA_PM_RESOLVER_H
