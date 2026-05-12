/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA DETERMINISTIC BUILD SYSTEM
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Guarantees:
 * - Same source → Same binary (bit-for-bit)
 * - Reproducible builds
 * - Hash verification
 * - Build caching
 */

#ifndef NOVA_DETERMINISTIC_H
#define NOVA_DETERMINISTIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════
// BUILD CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

// Opaque handle for build context - implementation hidden in .c file
typedef struct DeterministicBuildContext DeterministicBuildContext;

// ═══════════════════════════════════════════════════════════════════════════
// BUILD CACHE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  uint8_t source_hash[32];
  uint8_t output_hash[32];
  time_t timestamp;
  const char *output_path;
} CacheEntry;

// Opaque handle for build cache - implementation hidden in .c file
typedef struct BuildCache BuildCache;

/**
 * Create build cache
 */
BuildCache *build_cache_create(const char *cache_dir);

/**
 * Destroy build cache
 */
void build_cache_destroy(BuildCache *cache);

/**
 * Look up cached build
 */
CacheEntry *build_cache_lookup(BuildCache *cache,
                               const uint8_t source_hash[32]);

/**
 * Store build in cache
 */
void build_cache_store(BuildCache *cache, const uint8_t source_hash[32],
                       const uint8_t output_hash[32], const char *output_path);

/**
 * Verify cache entry
 */
bool build_cache_verify(BuildCache *cache, CacheEntry *entry);

// ═══════════════════════════════════════════════════════════════════════════
// DETERMINISTIC BUILD
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create deterministic build context
 */
DeterministicBuildContext *deterministic_build_create(void);

/**
 * Destroy build context
 */
void deterministic_build_destroy(DeterministicBuildContext *ctx);

/**
 * Set fixed timestamp (for reproducibility)
 */
void deterministic_set_timestamp(DeterministicBuildContext *ctx,
                                 time_t timestamp);

/**
 * Add build flag (will be sorted automatically)
 */
void deterministic_add_flag(DeterministicBuildContext *ctx, const char *flag);

/**
 * Compute source hash
 */
void deterministic_hash_source(DeterministicBuildContext *ctx,
                               const char *source, size_t len);

/**
 * Compute output hash
 */
void deterministic_hash_output(DeterministicBuildContext *ctx,
                               const uint8_t *output, size_t len);

/**
 * Verify build is deterministic
 */
bool deterministic_verify(DeterministicBuildContext *ctx,
                          const uint8_t expected_hash[32]);

/**
 * Build with determinism guarantees
 */
bool deterministic_build(DeterministicBuildContext *ctx,
                         const char *source_file, const char *output_file);

// ═══════════════════════════════════════════════════════════════════════════
// HASH UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * SHA-256 hash
 */
void sha256_hash(const uint8_t *data, size_t len, uint8_t hash[32]);

/**
 * Hash to hex string
 */
void hash_to_hex(const uint8_t hash[32], char hex[65]);

/**
 * Compare hashes
 */
bool hash_equals(const uint8_t hash1[32], const uint8_t hash2[32]);

// ═══════════════════════════════════════════════════════════════════════════
// ENVIRONMENT NORMALIZATION
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Get normalized compiler version
 */
const char *deterministic_compiler_version(void);

/**
 * Get normalized target triple
 */
const char *deterministic_target_triple(void);

/**
 * Sort flags for determinism
 */
void deterministic_sort_flags(char **flags, size_t count);

/**
 * Remove non-deterministic flags
 */
void deterministic_sanitize_flags(char **flags, size_t *count);

// ═══════════════════════════════════════════════════════════════════════════
// VERIFICATION
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Verify two builds are identical
 */
bool deterministic_verify_identical(const char *build1, const char *build2);

/**
 * Generate build manifest (for verification)
 */
void deterministic_generate_manifest(DeterministicBuildContext *ctx,
                                     const char *manifest_file);

/**
 * Verify against manifest
 */
bool deterministic_verify_manifest(const char *output_file,
                                   const char *manifest_file);

#endif // NOVA_DETERMINISTIC_H
