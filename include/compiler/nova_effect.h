/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA EFFECT SYSTEM - IO/Pure Tracking
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Gödel-aware effect system:
 * - Track IO vs Pure functions
 * - Effect inference
 * - Effect polymorphism
 * - Proof boundaries
 */

#ifndef NOVA_EFFECT_H
#define NOVA_EFFECT_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// EFFECT TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    EFFECT_PURE = 0,        // No side effects (provable)
    EFFECT_IO = 1 << 0,     // I/O operations
    EFFECT_MEMORY = 1 << 1, // Memory allocation
    EFFECT_THROW = 1 << 2,  // Can throw exceptions
    EFFECT_ASYNC = 1 << 3,  // Asynchronous operations
    EFFECT_UNSAFE = 1 << 4, // Unsafe operations (FFI, etc)
    EFFECT_UNKNOWN = 1 << 5 // Unknown effects (Gödel boundary)
} EffectKind;

typedef unsigned int EffectSet;

// ═══════════════════════════════════════════════════════════════════════════
// PROOF BOUNDARIES (Gödel-aware)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    PROOF_VERIFIED,   // Formally verified (SMT proven)
    PROOF_TRUSTED,    // Trusted but not proven
    PROOF_HEURISTIC,  // Heuristic optimization zone
    PROOF_UNKNOWN     // Beyond Gödel boundary
} ProofLevel;

typedef struct {
    ProofLevel level;
    const char *proof_source; // SMT solver, manual, etc
    const char *assumptions;  // What was assumed
    bool godel_incomplete;    // True if Gödel incompleteness applies
} ProofBoundary;

// ═══════════════════════════════════════════════════════════════════════════
// EFFECT ANNOTATIONS
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    EffectSet effects;
    ProofBoundary proof;
    bool inferred;    // True if inferred, false if explicit
    bool polymorphic; // True if effect-polymorphic
} EffectAnnotation;

// ═══════════════════════════════════════════════════════════════════════════
// EFFECT SYSTEM API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create effect annotation
 */
EffectAnnotation *effect_create(EffectSet effects, ProofLevel proof_level);

/**
 * Infer effects from function body
 */
EffectSet effect_infer(void *ast_node);

/**
 * Check if effects are compatible
 */
bool effect_compatible(EffectSet required, EffectSet provided);

/**
 * Combine effect sets
 */
EffectSet effect_union(EffectSet e1, EffectSet e2);

/**
 * Check if effect set is pure
 */
bool effect_is_pure(EffectSet effects);

/**
 * Mark proof boundary
 */
ProofBoundary *proof_boundary_create(ProofLevel level, const char *source);

/**
 * Check if within Gödel boundary
 */
bool proof_within_godel_boundary(ProofBoundary *boundary);

// ═══════════════════════════════════════════════════════════════════════════
// EFFECT INFERENCE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct EffectInferenceContext {
    void *ast;
    EffectSet current_effects;
    ProofLevel current_proof_level;
    bool in_pure_context;
} EffectInferenceContext;

/**
 * Initialize inference context
 */
EffectInferenceContext *effect_inference_init(void);

/**
 * Infer effects for entire module
 */
void effect_inference_run(EffectInferenceContext *ctx, void *module);

/**
 * Get inferred effects for function
 */
EffectSet effect_inference_get(EffectInferenceContext *ctx, const char *function_name);

// ═══════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Convert effect set to string (for debugging)
 */
const char *effect_to_string(EffectSet effects);

/**
 * Convert proof level to string
 */
const char *proof_level_to_string(ProofLevel level);

/**
 * Check if function call is allowed in current context
 */
bool effect_check_call(EffectSet caller_effects, EffectSet callee_effects);

#endif // NOVA_EFFECT_H
