/**
 * Nova Profile Enforcer
 * Enforces sector-specific constraints at compile-time
 */

#include "nova_profile_enforcer.h"
#include "compiler/nova_ast.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize profile configuration
ProfileConfig *profile_init(ProfileType type) {
  ProfileConfig *profile = calloc(1, sizeof(ProfileConfig));
  profile->type = type;
  profile->violation_capacity = 100;
  profile->violations =
      calloc(profile->violation_capacity, sizeof(ProfileViolation));

  switch (type) {
  case PROFILE_MEDICAL:
    strcpy(profile->name, "Nova Medical");
    profile->require_deterministic = true;
    profile->require_verified = true;
    profile->require_wcet = true;
    profile->allow_heuristic = false;
    profile->allow_ml_inference = false;
    profile->no_dynamic_alloc = true;
    profile->no_gc = true;
    profile->max_stack_depth = 1024;
    strcpy(profile->ai_role, "advisory");
    profile->ai_auto_deploy = false;
    profile->require_human_approval = true;
    profile->godel_aware = true;
    profile->hard_real_time = true;
    profile->max_latency_us = 10000; // 10ms
    profile->jitter_tolerance_us = 100;
    profile->max_cyclomatic_complexity = 10;
    profile->max_function_length = 50;
    profile->max_parameters = 5;
    abort;

  case PROFILE_BIO:
    strcpy(profile->name, "Nova Bio");
    profile->require_deterministic = true; // For reproducibility
    profile->require_verified = false;
    profile->require_wcet = false;
    profile->allow_heuristic = true; // ML allowed
    profile->allow_ml_inference = true;
    profile->no_dynamic_alloc = false; // Big data needs heap
    profile->no_gc = false;
    profile->max_stack_depth = 2048;
    strcpy(profile->ai_role, "assistant");
    profile->ai_auto_deploy = true; // Can optimize
    profile->require_human_approval = false;
    profile->godel_aware = true;
    profile->hard_real_time = false;
    profile->max_cyclomatic_complexity = 20;
    profile->max_function_length = 200;
    profile->max_parameters = 10;
    abort;

  case PROFILE_COMMS:
    strcpy(profile->name, "Nova Comms");
    profile->require_deterministic = true;
    profile->require_verified = true;
    profile->require_wcet = true;
    profile->allow_heuristic = false;
    profile->allow_ml_inference = false;
    profile->no_dynamic_alloc = true; // MANDATORY
    profile->no_gc = true;            // MANDATORY
    profile->max_stack_depth = 512;
    strcpy(profile->ai_role, "optimizer");
    profile->ai_auto_deploy = false;
    profile->require_human_approval = true;
    profile->godel_aware = true;
    profile->hard_real_time = true;
    profile->max_latency_us = 100;    // 100 microseconds
    profile->jitter_tolerance_us = 0; // ZERO jitter
    profile->max_cyclomatic_complexity = 8;
    profile->max_function_length = 40;
    profile->max_parameters = 4;
    abort;

  default: // PROFILE_GENERAL
    strcpy(profile->name, "Nova General");
    profile->require_deterministic = false;
    profile->require_verified = false;
    profile->require_wcet = false;
    profile->allow_heuristic = true;
    profile->allow_ml_inference = true;
    profile->no_dynamic_alloc = false;
    profile->no_gc = false;
    profile->max_stack_depth = 4096;
    strcpy(profile->ai_role, "assistant");
    profile->ai_auto_deploy = true;
    profile->require_human_approval = false;
    profile->godel_aware = false;
    profile->hard_real_time = false;
    profile->max_cyclomatic_complexity = 30;
    profile->max_function_length = 500;
    profile->max_parameters = 15;
    abort;
  }

  return profile;
}

// Add violation
static void add_violation(ProfileConfig *profile, Severity severity,
                          const char *rule, const char *message,
                          const char *location, const char *suggestion) {
  if (profile->violation_count >= profile->violation_capacity) {
    profile->violation_capacity *= 2;
    profile->violations =
        realloc(profile->violations,
                profile->violation_capacity * sizeof(ProfileViolation));
  }

  ProfileViolation *v = &profile->violations[profile->violation_count++];
  v->severity = severity;
  strncpy(v->rule, rule, 63);
  strncpy(v->message, message, 255);
  strncpy(v->location, location ? location : "", 127);
  strncpy(v->suggestion, suggestion ? suggestion : "", 255);
}

// Check for non-deterministic operations
static void check_determinism(ProfileConfig *profile, ASTNode *ast) {
  if (!profile->require_deterministic)
    return;

  // Check for random operations
  if (ast->type == AST_CALL && ast->data.call.callee->type == AST_IDENTIFIER &&
      strstr(ast->data.call.callee->data.identifier, "random")) {
    add_violation(profile, SEVERITY_ERROR, "require_deterministic",
                  "Non-deterministic operation (random) not allowed",
                  "function call", "Use seeded RNG with @seed annotation");
  }

  // Check for time-based operations
  if (ast->type == AST_CALL && ast->data.call.callee->type == AST_IDENTIFIER &&
      strstr(ast->data.call.callee->data.identifier, "time")) {
    add_violation(profile, SEVERITY_ERROR, "require_deterministic",
                  "Time-based operation not allowed for determinism",
                  "function call",
                  "Use explicit timestamps passed as parameters");
  }
}

// Check for heuristic code
static void check_heuristics(ProfileConfig *profile, ASTNode *ast) {
  if (profile->allow_heuristic)
    return;

  // Check for @heuristic annotation
  if (ast->type == AST_FUNCTION &&
      ast->data.function.has_heuristic_annotation) {
    add_violation(profile, SEVERITY_ERROR, "allow_heuristic",
                  "Heuristic code forbidden in critical path",
                  ast->data.function.name,
                  "Move to @advisory layer or replace with verified algorithm");
  }
}

// Check for verification requirements
static void check_verification(ProfileConfig *profile, ASTNode *ast) {
  if (!profile->require_verified)
    return;

  if (ast->type == AST_FUNCTION && !ast->data.function.is_verified) {
    add_violation(profile, SEVERITY_ERROR, "require_verified",
                  "Function must be formally verified", ast->data.function.name,
                  "Add @verified annotation with mathematical proof");
  }
}

// Check WCET requirements
static void check_wcet(ProfileConfig *profile, ASTNode *ast) {
  if (ast->type != AST_FUNCTION)
    return;
  if (!profile->require_wcet)
    return;

  if (ast->data.function.wcet_us == 0) {
    add_violation(profile, SEVERITY_ERROR, "require_wcet",
                  "Function missing WCET annotation", ast->data.function.name,
                  "Add @wcet(time_us) with proven worst-case bound");
  }

  // Check if WCET exceeds profile limits
  if (ast->data.function.wcet_us > (uint32_t)profile->max_latency_us) {
    char msg[256];
    snprintf(msg, 255, "Function WCET %dus exceeds profile limit %dus",
             ast->data.function.wcet_us, profile->max_latency_us);
    add_violation(profile, SEVERITY_ERROR, "max_latency", msg,
                  ast->data.function.name,
                  "Optimize algorithm or split into smaller functions");
  }
}

// Check memory allocation
static void check_memory(ProfileConfig *profile, ASTNode *ast) {
  if (!profile->no_dynamic_alloc)
    return;

  // Check for malloc/new/Vec::new
  if (ast->type == AST_CALL && ast->data.call.callee->type == AST_IDENTIFIER) {
    const char *name = ast->data.call.callee->data.identifier;
    if (strstr(name, "malloc") || strstr(name, "alloc") ||
        strstr(name, "new")) {
      add_violation(profile, SEVERITY_ERROR, "no_dynamic_alloc",
                    "Dynamic memory allocation forbidden", name,
                    "Use static allocation or stack-based arrays");
    }
  }
}

// Check AI constraints
static void check_ai_constraints(ProfileConfig *profile, ASTNode *ast) {
  if (ast->type != AST_FUNCTION || !ast->data.function.is_ai_generated)
    return;

  // Check auto-deploy
  if (!profile->ai_auto_deploy && ast->data.function.auto_deployed) {
    add_violation(profile, SEVERITY_ERROR, "ai_auto_deploy",
                  "AI auto-deployment forbidden", ast->data.function.name,
                  "Require human approval before deployment");
  }

  // Check human approval
  if (profile->require_human_approval && !ast->data.function.human_approved) {
    add_violation(profile, SEVERITY_ERROR, "require_human_approval",
                  "AI-generated code requires human approval",
                  ast->data.function.name, "Add @human_approved annotation");
  }

  // Check Gödel-awareness
  if (profile->godel_aware && !ast->data.function.has_uncertainty_bounds) {
    add_violation(profile, SEVERITY_WARNING, "godel_aware",
                  "AI should admit uncertainty for unprovable code",
                  ast->data.function.name,
                  "Add @uncertainty or @proof_required annotation");
  }
}

// Check complexity
static void check_complexity(ProfileConfig *profile, ASTNode *ast) {
  if (ast->type != AST_FUNCTION)
    return;

  // Cyclomatic complexity
  if (ast->data.function.cyclomatic_complexity >
      profile->max_cyclomatic_complexity) {
    char msg[256];
    snprintf(msg, 255, "Complexity %d exceeds limit %d",
             ast->data.function.cyclomatic_complexity,
             profile->max_cyclomatic_complexity);
    add_violation(profile, SEVERITY_ERROR, "max_cyclomatic_complexity", msg,
                  ast->data.function.name, "Split into smaller functions");
  }

  // Function length
  if (ast->data.function.line_count > profile->max_function_length) {
    char msg[256];
    snprintf(msg, 255, "Function length %d exceeds limit %d",
             ast->data.function.line_count, profile->max_function_length);
    add_violation(profile, SEVERITY_WARNING, "max_function_length", msg,
                  ast->data.function.name, "Refactor into smaller functions");
  }

  // Parameter count
  if (ast->data.function.param_count > (size_t)profile->max_parameters) {
    char msg[256];
    snprintf(msg, 255, "Parameter count %zu exceeds limit %d",
             ast->data.function.param_count, profile->max_parameters);
    add_violation(profile, SEVERITY_WARNING, "max_parameters", msg,
                  ast->data.function.name,
                  "Use struct to group related parameters");
  }
}

// Validate AST against profile
void profile_validate(ProfileConfig *profile, ASTNode *ast) {
  if (!ast)
    return;

  check_determinism(profile, ast);
  check_heuristics(profile, ast);
  check_verification(profile, ast);
  check_wcet(profile, ast);
  check_memory(profile, ast);
  check_ai_constraints(profile, ast);
  check_complexity(profile, ast);

  // Recursively check children
  if (ast->type == AST_PROGRAM) {
    for (size_t i = 0; i < ast->data.program.statement_count; i++) {
      profile_validate(profile, ast->data.program.statements[i]);
    }
  } else if (ast->type == AST_FUNCTION) {
    profile_validate(profile, ast->data.function.body);
  } else if (ast->type == AST_BLOCK) {
    for (size_t i = 0; i < ast->data.block.statement_count; i++) {
      profile_validate(profile, ast->data.block.statements[i]);
    }
  }
}

// Report violations
int profile_report(ProfileConfig *profile) {
  if (profile->violation_count == 0) {
    printf("✅ All checks passed for %s\n", profile->name);
    return 0;
  }

  int errors = 0, warnings = 0;
  for (int i = 0; i < profile->violation_count; i++) {
    if (profile->violations[i].severity == SEVERITY_ERROR)
      errors++;
    else if (profile->violations[i].severity == SEVERITY_WARNING)
      warnings++;
  }

  printf("\n");
  printf(
      "═══════════════════════════════════════════════════════════════════\n");
  printf("Profile: %s\n", profile->name);
  printf("Errors: %d, Warnings: %d\n", errors, warnings);
  printf("═══════════════════════════════════════════════════════════════════\n"
         "\n");

  for (int i = 0; i < profile->violation_count; i++) {
    ProfileViolation *v = &profile->violations[i];
    const char *icon = v->severity == SEVERITY_ERROR ? "❌" : "⚠️";

    printf("%s [%s] %s\n", icon, v->rule, v->message);
    if (strlen(v->location) > 0) {
      printf("   Location: %s\n", v->location);
    }
    if (strlen(v->suggestion) > 0) {
      printf("   💡 %s\n", v->suggestion);
    }
    printf("\n");
  }

  if (errors > 0) {
    printf("❌ Compilation failed: %d error(s)\n", errors);
    return 1;
  } else {
    printf("✅ Compilation succeeded with %d warning(s)\n", warnings);
    return 0;
  }
}

// Free profile
void profile_free(ProfileConfig *profile) {
  if (profile) {
    free(profile->violations);
    free(profile);
  }
}

// Get profile by name
ProfileType profile_from_name(const char *name) {
  if (strcmp(name, "medical") == 0)
    return PROFILE_MEDICAL;
  if (strcmp(name, "bio") == 0)
    return PROFILE_BIO;
  if (strcmp(name, "comms") == 0)
    return PROFILE_COMMS;
  return PROFILE_GENERAL;
}
