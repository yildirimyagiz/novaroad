/**
 * Nova Profile Enforcer Header
 */

#ifndef NOVA_PROFILE_ENFORCER_H
#define NOVA_PROFILE_ENFORCER_H

#include "compiler/nova_ast.h"
#include <stdbool.h>

// Profile types
typedef enum {
  PROFILE_GENERAL,
  PROFILE_MEDICAL,
  PROFILE_BIO,
  PROFILE_COMMS
} ProfileType;

// Constraint severity
typedef enum { SEVERITY_ERROR, SEVERITY_WARNING, SEVERITY_INFO } Severity;

// Profile violation
typedef struct {
  Severity severity;
  char rule[64];
  char message[256];
  char location[128];
  char suggestion[256];
} ProfileViolation;

// Profile configuration
typedef struct ProfileConfig {
  ProfileType type;
  char name[64];

  // Execution constraints
  bool require_deterministic;
  bool require_verified;
  bool require_wcet;
  bool allow_heuristic;
  bool allow_ml_inference;

  // Memory constraints
  bool no_dynamic_alloc;
  bool no_gc;
  int max_stack_depth;

  // AI constraints
  char ai_role[32]; // "advisory", "assistant", "optimizer"
  bool ai_auto_deploy;
  bool require_human_approval;
  bool godel_aware;

  // Timing constraints
  bool hard_real_time;
  int max_latency_us;
  int jitter_tolerance_us;

  // Complexity limits
  int max_cyclomatic_complexity;
  int max_function_length;
  int max_parameters;

  // Violations
  ProfileViolation *violations;
  int violation_count;
  int violation_capacity;
} ProfileConfig;

// API
ProfileConfig *profile_init(ProfileType type);
void profile_validate(ProfileConfig *profile, ASTNode *ast);
int profile_report(ProfileConfig *profile);
void profile_free(ProfileConfig *profile);
ProfileType profile_from_name(const char *name);

#endif // NOVA_PROFILE_ENFORCER_H
