/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA COMPILATION MODES                                   ║
 * ║                                                                               ║
 * ║  Week 11-12: Three compilation modes                                         ║
 * ║  - GC Mode: Easy, Python-like (default)                                      ║
 * ║  - Hybrid Mode: GC + borrow checker hints                                    ║
 * ║  - Borrow Mode: Rust-like, zero GC overhead                                  ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_MODE_H
#define NOVA_MODE_H

#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════════
// COMPILATION MODES
// ═══════════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_MODE_GC,      // GC mode (default, easy)
    NOVA_MODE_HYBRID,  // Hybrid (GC + borrow checker)
    NOVA_MODE_BORROW,  // Borrow mode (Rust-like, no GC)
} NovaCompilationMode;

// ═══════════════════════════════════════════════════════════════════════════════
// MODE CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    NovaCompilationMode mode;
    
    // GC options
    bool enable_gc;
    bool gc_generational;
    
    // Borrow checker options
    bool enable_borrow_checker;
    bool borrow_checker_strict;
    
    // Optimization options
    bool aggressive_inline;
    bool zero_cost_abstractions;
    bool escape_analysis;
    
} NovaModeConfig;

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Get default config for a mode
 */
NovaModeConfig nova_mode_get_config(NovaCompilationMode mode);

/**
 * Parse mode from string
 */
NovaCompilationMode nova_mode_from_string(const char *mode_str);

/**
 * Get mode name
 */
const char *nova_mode_to_string(NovaCompilationMode mode);

/**
 * Check if GC is enabled for this mode
 */
bool nova_mode_uses_gc(NovaCompilationMode mode);

/**
 * Check if borrow checker is enabled
 */
bool nova_mode_uses_borrow_checker(NovaCompilationMode mode);

#endif // NOVA_MODE_H
