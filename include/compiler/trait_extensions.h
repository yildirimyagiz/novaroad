// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA TRAIT SYSTEM EXTENSIONS BACKEND  v2.0                            ║
// ║  Associated Types, Higher-Kinded Traits, and Trait Objects              ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_TRAIT_EXTENSIONS_H
#define NOVA_TRAIT_EXTENSIONS_H

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

typedef struct nova_trait_extensions nova_trait_extensions_t;

typedef struct {
    size_t associated_type_count;
    size_t hkt_trait_count;
    size_t trait_object_count;
    size_t specialization_count;
} nova_trait_stats_t;

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// Create a new trait extensions context
nova_trait_extensions_t *nova_trait_extensions_create(void);

/// Destroy trait extensions context
void nova_trait_extensions_destroy(nova_trait_extensions_t *ext);

/// Add an associated type implementation to a trait
int nova_trait_extensions_add_associated_type_impl(nova_trait_extensions_t *ext,
                                                   const char *trait_name,
                                                   const char *impl_type_name,
                                                   const char *assoc_type_name,
                                                   nova_type_t *resolved_type);

/// Lookup an associated type in a trait implementation
nova_type_t *nova_trait_extensions_lookup_associated_type(nova_trait_extensions_t *ext,
                                                          const char *trait_name,
                                                          const char *impl_type_name,
                                                          const char *assoc_type_name);

/// Add a Higher-Kinded Type (HKT) trait definition
int nova_trait_extensions_add_hkt_trait(nova_trait_extensions_t *ext, const char *trait_name,
                                        size_t arity);

/// Get trait system statistics
size_t nova_trait_extensions_get_stats(nova_trait_stats_t *stats);

#endif /* NOVA_TRAIT_EXTENSIONS_H */
