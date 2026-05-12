/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_incremental.h - Incremental Compilation & Caching
 * ═══════════════════════════════════════════════════════════════════════════
 * Performance target: 90% reduction in re-compile time
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_INCREMENTAL_H
#define NOVA_INCREMENTAL_H

#include "../memory/nova_arena.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hash type
typedef uint64_t NovaHash;

// Cache entry for a file or function
typedef struct {
  NovaHash source_hash;
  NovaHash config_hash; // Combined hash of flags, target, and environment
  const char *source_path;
  const char *object_path;

  // Dependencies
  NovaHash *dep_hashes;
  const char **dep_paths;
  size_t dep_count;

  bool is_valid;
} NovaCacheEntry;

// Incremental compilation manager
typedef struct {
  const char *cache_dir;
  Arena *arena;
  NovaCacheEntry *entries;
  size_t entry_count;
  size_t entry_capacity;
} NovaIncrementalContext;

// Initialize incremental context
NovaIncrementalContext *nova_incremental_create(const char *cache_dir);
void nova_incremental_destroy(NovaIncrementalContext *ctx);

// Calculate hash for a buffer
NovaHash nova_hash_buffer(const void *buffer, size_t size);

// Check if a file needs re-compilation
bool nova_incremental_needs_compile(NovaIncrementalContext *ctx,
                                      const char *source_path,
                                      NovaHash current_hash,
                                      NovaHash config_hash);

// Record a successful compilation
void nova_incremental_update_cache(
    NovaIncrementalContext *ctx, const char *source_path,
    NovaHash source_hash, NovaHash config_hash, const char *object_path,
    const char **dep_paths, NovaHash *dep_hashes, size_t dep_count);

// Load/Save manifest
bool nova_incremental_load_manifest(NovaIncrementalContext *ctx);
bool nova_incremental_save_manifest(NovaIncrementalContext *ctx);

#endif // NOVA_INCREMENTAL_H
