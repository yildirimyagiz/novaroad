/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA PROOF SYSTEM - Gödel-Aware Verification Boundaries
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * "We can prove what we can prove, acknowledge what we cannot,
 *  and separate heuristics from verified facts." - Gödel-inspired design
 */

#ifndef NOVA_PROOF_H
#define NOVA_PROOF_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// PROOF LEVELS (Gödel Hierarchy)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    /**
     * VERIFIED: Formally proven by SMT solver
     * - Decidable properties only
     * - Within Gödel's boundaries
     * - 100% confidence
     */
    PROOF_VERIFIED = 0,
    
    /**
     * TRUSTED: Assumed correct but not proven
     * - Axioms, library functions
     * - Explicit trust boundary
     * - Must be marked clearly
     */
    PROOF_TRUSTED = 1,
    
    /**
     * HEURISTIC: Optimization zone
     * - Performance optimizations
     * - Not proven correct
     * - Can be disabled for verification
     */
    PROOF_HEURISTIC = 2,
    
    /**
     * UNKNOWN: Beyond Gödel boundary
     * - Undecidable properties
     * - Halting problem, etc.
     * - Explicit acknowledgment of limits
     */
    PROOF_UNKNOWN = 3
} ProofLevel;

// ═══════════════════════════════════════════════════════════════════════════
// VERIFICATION ZONES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    const char *name;
    ProofLevel level;
    const char *proof_method;    // "Z3", "manual", "trusted", etc
    const char *assumptions;      // What we assumed
    bool godel_incomplete;        // Affected by incompleteness theorem
    bool decidable;               // Is this property decidable?
    size_t verification_time_ms;  // How long verification took
} VerificationZone;

// ═══════════════════════════════════════════════════════════════════════════
// BOUNDARY MARKERS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Mark boundary between verified and unverified code
 */
#define PROOF_BOUNDARY_VERIFIED() \
    __proof_boundary_mark(__FILE__, __LINE__, PROOF_VERIFIED)

#define PROOF_BOUNDARY_TRUSTED() \
    __proof_boundary_mark(__FILE__, __LINE__, PROOF_TRUSTED)

#define PROOF_BOUNDARY_HEURISTIC() \
    __proof_boundary_mark(__FILE__, __LINE__, PROOF_HEURISTIC)

#define PROOF_BOUNDARY_UNKNOWN() \
    __proof_boundary_mark(__FILE__, __LINE__, PROOF_UNKNOWN)

/**
 * Internal boundary marker
 */
void __proof_boundary_mark(const char *file, int line, ProofLevel level);

// ═══════════════════════════════════════════════════════════════════════════
// VERIFICATION CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct ProofContext {
    VerificationZone *zones;
    size_t zone_count;
    size_t zone_capacity;
    
    ProofLevel current_level;
    bool strict_mode;  // Reject anything not verified
    bool godel_aware;  // Acknowledge incompleteness
} ProofContext;

/**
 * Create proof context
 */
ProofContext *proof_context_create(void);

/**
 * Destroy proof context
 */
void proof_context_destroy(ProofContext *ctx);

/**
 * Enter verification zone
 */
void proof_enter_zone(ProofContext *ctx, const char *name, ProofLevel level);

/**
 * Exit verification zone
 */
void proof_exit_zone(ProofContext *ctx);

/**
 * Check if property is verifiable
 */
bool proof_is_decidable(ProofContext *ctx, const char *property);

/**
 * Mark Gödel boundary
 */
void proof_mark_godel_boundary(ProofContext *ctx, const char *reason);

// ═══════════════════════════════════════════════════════════════════════════
// VERIFICATION PROPERTIES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    PROPERTY_MEMORY_SAFETY,      // Decidable
    PROPERTY_TYPE_SAFETY,        // Decidable
    PROPERTY_TERMINATION,        // Undecidable (Halting problem)
    PROPERTY_BOUNDS_CHECK,       // Decidable
    PROPERTY_NULL_SAFETY,        // Decidable
    PROPERTY_DATA_RACE_FREEDOM,  // Decidable (with ownership)
    PROPERTY_FUNCTIONAL_CORRECTNESS, // Often undecidable
} VerificationProperty;

/**
 * Check if property is decidable
 */
bool property_is_decidable(VerificationProperty prop);

/**
 * Get property name
 */
const char *property_name(VerificationProperty prop);

// ═══════════════════════════════════════════════════════════════════════════
// HEURISTIC ZONE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    const char *name;
    const char *description;
    bool can_disable;      // Can be turned off for verification
    bool affects_semantics; // Does it change program behavior?
    double confidence;     // 0.0 to 1.0
} HeuristicZone;

/**
 * Create heuristic zone
 */
HeuristicZone *heuristic_zone_create(const char *name, const char *desc);

/**
 * Register heuristic optimization
 */
void heuristic_register(ProofContext *ctx, HeuristicZone *zone);

/**
 * Check if in heuristic zone
 */
bool proof_in_heuristic_zone(ProofContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// GÖDEL AWARENESS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Acknowledge Gödel incompleteness
 * 
 * "There exist true statements about this program that cannot be proven
 *  within this system" - Gödel's Incompleteness Theorem
 */
void proof_acknowledge_godel_limits(ProofContext *ctx);

/**
 * Check if statement is within decidable fragment
 */
bool proof_is_within_decidable_fragment(ProofContext *ctx, const char *statement);

/**
 * Explain why property is undecidable
 */
const char *proof_explain_undecidability(VerificationProperty prop);

// ═══════════════════════════════════════════════════════════════════════════
// REPORTING
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Generate verification report
 */
void proof_generate_report(ProofContext *ctx, const char *output_file);

/**
 * Print verification summary
 */
void proof_print_summary(ProofContext *ctx);

/**
 * Get verification coverage
 * Returns percentage of code that is verified (0.0 to 1.0)
 */
double proof_get_coverage(ProofContext *ctx);

#endif // NOVA_PROOF_H
