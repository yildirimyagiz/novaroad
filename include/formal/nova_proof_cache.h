/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_proof_cache.h - Zero-Overhead Runtime Proof Cache
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Strategy: Load proofs ONCE at startup, then use O(1) boolean flags.
 * 
 * Performance:
 *   - NOVA_FORMAL_DISABLED: 0% overhead (all checks compiled out)
 *   - NOVA_FORMAL_FAST:     <1% overhead (inline boolean checks)
 *   - NOVA_FORMAL_RESEARCH: 5% overhead (detailed logging)
 *   - NOVA_FORMAL_STRICT:   Build-time only (abort on missing proof)
 */

#ifndef NOVA_PROOF_CACHE_H
#define NOVA_PROOF_CACHE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Proof cache: Populated once at startup
 */
typedef struct {
  // Core invariants (Isabelle)
  bool tensor_safety_proven;          // Tensor shape/size consistency
  bool autodiff_correctness_proven;   // Backprop math correctness
  bool kernel_determinism_proven;     // Reproducible execution
  
  // Memory safety (KLEE)
  bool memory_safety_proven;          // No buffer overflows
  bool allocation_safety_proven;      // No double-free/use-after-free
  
  // Optimizer stability (CVC5)
  bool optimizer_stable_proven;       // Learning rate stability
  bool convergence_guaranteed_proven; // Loss descent guarantee
  
  // Hardware correctness (manual verification + testing)
  bool gpu_kernel_verified;
  bool distributed_comm_verified;
} NovaProofCache;

/**
 * Initialize cache (call ONCE at program startup)
 */
void nova_proof_cache_init(void);

/**
 * Fast accessors (inline for zero overhead)
 */
const NovaProofCache* nova_proof_cache_get(void);

static inline bool nova_is_tensor_safety_proven(void) {
  extern NovaProofCache __nova_global_proof_cache;
  return __nova_global_proof_cache.tensor_safety_proven;
}

static inline bool nova_is_memory_safety_proven(void) {
  extern NovaProofCache __nova_global_proof_cache;
  return __nova_global_proof_cache.memory_safety_proven;
}

static inline bool nova_is_optimizer_stable_proven(void) {
  extern NovaProofCache __nova_global_proof_cache;
  return __nova_global_proof_cache.optimizer_stable_proven;
}

static inline bool nova_is_kernel_determinism_proven(void) {
  extern NovaProofCache __nova_global_proof_cache;
  return __nova_global_proof_cache.kernel_determinism_proven;
}

/**
 * Assertion macros with proof context
 */
#ifdef NOVA_FORMAL_STRICT
  #define NOVA_ASSERT_PROVEN(condition, proof_name) \
    do { \
      if (!(condition)) { \
        fprintf(stderr, "❌ [Gödel] PROOF VIOLATION: %s\n", proof_name); \
        fprintf(stderr, "   Location: %s:%d in %s()\n", \
                __FILE__, __LINE__, __func__); \
        fprintf(stderr, "   Condition: %s\n", #condition); \
        abort(); \
      } \
    } while(0)

#elif defined(NOVA_FORMAL_FAST)
  #define NOVA_ASSERT_PROVEN(condition, proof_name) \
    do { \
      if (!(condition)) { \
        fprintf(stderr, "⚠️ [Gödel] Warning: %s violation\n", proof_name); \
        fprintf(stderr, "   at %s:%d\n", __FILE__, __LINE__); \
      } \
    } while(0)

#else
  // DISABLED: Zero overhead
  #define NOVA_ASSERT_PROVEN(condition, proof_name) ((void)0)
#endif

/**
 * Panic macro for unrecoverable proof violations
 */
#define nova_panic(fmt, ...) \
  do { \
    fprintf(stderr, "💀 [Gödel] PANIC: " fmt "\n", ##__VA_ARGS__); \
    fprintf(stderr, "   at %s:%d in %s()\n", __FILE__, __LINE__, __func__); \
    abort(); \
  } while(0)

/**
 * Proof-backed precondition check
 */
#define NOVA_REQUIRE_PROVEN(proof_flag, proof_name) \
  do { \
    if (!(proof_flag)) { \
      nova_panic("Required proof '%s' is missing!", proof_name); \
    } \
  } while(0)

#endif // NOVA_PROOF_CACHE_H
