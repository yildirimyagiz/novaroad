/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA RUNTIME-FREE PATH
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Compile to pure native code with NO runtime dependency:
 * - No GC (compile-time memory management)
 * - No interpreter
 * - No dynamic dispatch
 * - Bare metal capable
 * - Embedded systems ready
 */

#ifndef NOVA_RUNTIME_FREE_H
#define NOVA_RUNTIME_FREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h> /* Added for uint8_t support */

// ═══════════════════════════════════════════════════════════════════════════
// COMPILATION MODES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  RUNTIME_MODE_FULL,    // Full runtime (GC, reflection, etc)
  RUNTIME_MODE_MINIMAL, // Minimal runtime (basic allocator only)
  RUNTIME_MODE_NONE,    // No runtime (static only, bare metal)
  RUNTIME_MODE_CUSTOM   // Custom runtime subset
} RuntimeMode;

typedef struct {
  RuntimeMode mode;

  // Feature flags
  bool enable_gc;               // Garbage collector
  bool enable_reflection;       // Runtime reflection
  bool enable_exceptions;       // Exception handling
  bool enable_async;            // Async runtime
  bool enable_dynamic_dispatch; // Virtual calls

  // Memory
  bool static_memory_only;   // No malloc/free
  size_t static_memory_size; // Static memory pool size

  // Target
  bool bare_metal; // Bare metal target (no OS)
  bool embedded;   // Embedded system

  // Verification
  bool prove_no_alloc;      // Prove no heap allocation
  bool prove_bounded_stack; // Prove stack usage bounded
} RuntimeFreeConfig;

// ═══════════════════════════════════════════════════════════════════════════
// STATIC ANALYSIS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Verify code can run without runtime
 */
bool runtime_free_verify(void *ast, RuntimeFreeConfig *config);

/**
 * Check if function requires GC
 */
bool runtime_free_needs_gc(void *function);

/**
 * Check if function needs heap allocation
 */
bool runtime_free_needs_heap(void *function);

/**
 * Compute maximum stack usage
 */
size_t runtime_free_max_stack_size(void *function);

/**
 * Verify all allocations are static
 */
bool runtime_free_all_static(void *module);

// ═══════════════════════════════════════════════════════════════════════════
// CODE GENERATION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct RuntimeFreeCodeGen RuntimeFreeCodeGen;

/**
 * Create runtime-free code generator
 */
RuntimeFreeCodeGen *runtime_free_codegen_create(RuntimeFreeConfig *config);

/**
 * Destroy code generator
 */
void runtime_free_codegen_destroy(RuntimeFreeCodeGen *cg);

/**
 * Generate runtime-free code
 */
bool runtime_free_codegen_generate(RuntimeFreeCodeGen *cg, void *ast,
                                   const char *output);

/**
 * Generate bare metal startup code
 */
bool runtime_free_generate_startup(RuntimeFreeCodeGen *cg, const char *output);

// ═══════════════════════════════════════════════════════════════════════════
// MEMORY MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Static memory allocator
 */
typedef struct {
  uint8_t *pool;
  size_t pool_size;
  size_t allocated;
  bool overflow;
} StaticAllocator;

/**
 * Create static allocator
 */
StaticAllocator *static_allocator_create(size_t size);

/**
 * Allocate from static pool
 */
void *static_alloc(StaticAllocator *alloc, size_t size);

/**
 * Reset static allocator (arena style)
 */
void static_allocator_reset(StaticAllocator *alloc);

/**
 * Check if allocation would fit
 */
bool static_allocator_can_fit(StaticAllocator *alloc, size_t size);

// ═══════════════════════════════════════════════════════════════════════════
// COMPILE-TIME MEMORY MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Perform escape analysis
 */
typedef struct {
  bool escapes;       // Does value escape function?
  bool heap_required; // Must be heap allocated?
  bool stack_ok;      // Can be stack allocated?
  size_t lifetime;    // Lifetime in scopes
} EscapeAnalysis;

/**
 * Analyze if value escapes
 */
EscapeAnalysis runtime_free_escape_analysis(void *expr);

/**
 * Convert heap allocation to stack
 */
bool runtime_free_stack_promote(void *allocation);

/**
 * Compute static lifetime
 */
size_t runtime_free_lifetime(void *value);

// ═══════════════════════════════════════════════════════════════════════════
// BARE METAL SUPPORT
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Generate bare metal entry point
 */
void runtime_free_generate_bare_metal_entry(RuntimeFreeCodeGen *cg);

/**
 * Generate interrupt vector table
 */
void runtime_free_generate_ivt(RuntimeFreeCodeGen *cg, const char *output);

/**
 * Generate memory map
 */
void runtime_free_generate_memory_map(RuntimeFreeCodeGen *cg,
                                      const char *output);

/**
 * Linker script generation
 */
void runtime_free_generate_linker_script(RuntimeFreeCodeGen *cg,
                                         const char *output);

// ═══════════════════════════════════════════════════════════════════════════
// VERIFICATION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  bool has_gc_calls;
  bool has_heap_alloc;
  bool has_dynamic_dispatch;
  bool has_exceptions;
  bool has_unbounded_recursion;
  size_t max_stack_usage;
  size_t max_static_memory;
} RuntimeFreeReport;

/**
 * Generate runtime-free report
 */
RuntimeFreeReport runtime_free_analyze(void *module);

/**
 * Print runtime-free report
 */
void runtime_free_print_report(RuntimeFreeReport *report);

/**
 * Verify runtime-free constraints
 */
bool runtime_free_verify_constraints(RuntimeFreeReport *report,
                                     RuntimeFreeConfig *config);

// ═══════════════════════════════════════════════════════════════════════════
// UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Check if target supports runtime-free
 */
bool runtime_free_target_supported(const char *target);

/**
 * Get recommended config for target
 */
RuntimeFreeConfig runtime_free_recommended_config(const char *target);

/**
 * Estimate binary size (no runtime overhead)
 */
size_t runtime_free_estimate_size(void *module);

#endif // NOVA_RUNTIME_FREE_H
