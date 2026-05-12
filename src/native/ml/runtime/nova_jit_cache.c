/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_jit_cache.c - JIT-level Persistent Compilation Cache (ENHANCED)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_incremental.h"
#include "nova_jit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

// ═══════════════════════════════════════════════════════════════════════════
// CACHE ENTRY
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaHash ir_hash;
  void *machine_code;
  size_t code_size;
  time_t timestamp;
  uint32_t access_count;
  char *function_name;
} JITCacheEntry;

typedef struct {
  JITCacheEntry *entries;
  size_t entry_count;
  size_t entry_capacity;
  
  // Statistics
  size_t cache_hits;
  size_t cache_misses;
  size_t total_code_size;
  
  // Configuration
  size_t max_cache_size;  // bytes
  size_t max_entries;
  bool persistence_enabled;
} JITCache;

static JITCache *global_jit_cache = NULL;

// ═══════════════════════════════════════════════════════════════════════════
// CACHE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

static void ensure_cache_directory(void) {
  struct stat st = {0};
  if (stat(".nova_cache", &st) == -1) {
    mkdir(".nova_cache", 0700);
  }
}

static JITCache *jit_cache_create(void) {
  JITCache *cache = (JITCache *)calloc(1, sizeof(JITCache));
  cache->entry_capacity = 256;
  cache->entries = (JITCacheEntry *)calloc(cache->entry_capacity, sizeof(JITCacheEntry));
  cache->max_cache_size = 100 * 1024 * 1024;  // 100 MB
  cache->max_entries = 1000;
  cache->persistence_enabled = true;
  
  ensure_cache_directory();
  
  return cache;
}

static JITCache *get_global_cache(void) {
  if (!global_jit_cache) {
    global_jit_cache = jit_cache_create();
  }
  return global_jit_cache;
}

// ═══════════════════════════════════════════════════════════════════════════
// CACHE OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

static JITCacheEntry *find_entry(JITCache *cache, NovaHash ir_hash) {
  for (size_t i = 0; i < cache->entry_count; i++) {
    if (cache->entries[i].ir_hash == ir_hash) {
      return &cache->entries[i];
    }
  }
  return NULL;
}

static void evict_lru_entry(JITCache *cache) {
  if (cache->entry_count == 0) return;
  
  // Find least recently used entry
  size_t lru_idx = 0;
  time_t oldest = cache->entries[0].timestamp;
  
  for (size_t i = 1; i < cache->entry_count; i++) {
    if (cache->entries[i].timestamp < oldest) {
      oldest = cache->entries[i].timestamp;
      lru_idx = i;
    }
  }
  
  // Free entry
  free(cache->entries[lru_idx].machine_code);
  free(cache->entries[lru_idx].function_name);
  
  cache->total_code_size -= cache->entries[lru_idx].code_size;
  
  // Remove from array
  memmove(&cache->entries[lru_idx], &cache->entries[lru_idx + 1],
          (cache->entry_count - lru_idx - 1) * sizeof(JITCacheEntry));
  cache->entry_count--;
}

static void add_entry(JITCache *cache, NovaHash ir_hash, const char *fn_name,
                      void *code, size_t size) {
  // Check limits
  while (cache->entry_count >= cache->max_entries ||
         cache->total_code_size + size > cache->max_cache_size) {
    evict_lru_entry(cache);
  }
  
  // Expand if needed
  if (cache->entry_count >= cache->entry_capacity) {
    cache->entry_capacity *= 2;
    cache->entries = (JITCacheEntry *)realloc(cache->entries,
                                               cache->entry_capacity * sizeof(JITCacheEntry));
  }
  
  // Add new entry
  JITCacheEntry *entry = &cache->entries[cache->entry_count++];
  entry->ir_hash = ir_hash;
  entry->machine_code = malloc(size);
  memcpy(entry->machine_code, code, size);
  entry->code_size = size;
  entry->timestamp = time(NULL);
  entry->access_count = 0;
  entry->function_name = fn_name ? strdup(fn_name) : NULL;
  
  cache->total_code_size += size;
}

// ═══════════════════════════════════════════════════════════════════════════
// PERSISTENCE
// ═══════════════════════════════════════════════════════════════════════════

static void save_to_disk(NovaHash ir_hash, void *code, size_t size) {
  char path[1024];
  snprintf(path, sizeof(path), ".nova_cache/jit_%llx.bin", (unsigned long long)ir_hash);
  
  FILE *f = fopen(path, "wb");
  if (!f) return;
  
  // Write metadata
  fwrite(&size, sizeof(size_t), 1, f);
  time_t now = time(NULL);
  fwrite(&now, sizeof(time_t), 1, f);
  
  // Write code
  fwrite(code, 1, size, f);
  fclose(f);
}

static void *load_from_disk(NovaHash ir_hash, size_t *out_size) {
  char path[1024];
  snprintf(path, sizeof(path), ".nova_cache/jit_%llx.bin", (unsigned long long)ir_hash);
  
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  
  // Read metadata
  size_t size;
  time_t timestamp;
  fread(&size, sizeof(size_t), 1, f);
  fread(&timestamp, sizeof(time_t), 1, f);
  
  // Check if cache is stale (older than 24 hours)
  time_t now = time(NULL);
  if (now - timestamp > 86400) {
    fclose(f);
    return NULL;
  }
  
  // Read code
  void *code = malloc(size);
  fread(code, 1, size, f);
  fclose(f);
  
  *out_size = size;
  return code;
}

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════

void *nova_jit_get_cached_fn(NovaJITContext *ctx, const char *fn_name,
                               NovaHash ir_hash) {
  JITCache *cache = get_global_cache();
  
  // Check memory cache
  JITCacheEntry *entry = find_entry(cache, ir_hash);
  if (entry) {
    entry->timestamp = time(NULL);
    entry->access_count++;
    cache->cache_hits++;
    
    printf("JIT_CACHE: HIT for '%s' (hash %llx) - %u accesses\n",
           fn_name, (unsigned long long)ir_hash, entry->access_count);
    
    return entry->machine_code;
  }
  
  // Check disk cache
  if (cache->persistence_enabled) {
    size_t size;
    void *code = load_from_disk(ir_hash, &size);
    if (code) {
      printf("JIT_CACHE: Loaded from disk '%s' (hash %llx)\n",
             fn_name, (unsigned long long)ir_hash);
      
      add_entry(cache, ir_hash, fn_name, code, size);
      cache->cache_hits++;
      
      return code;
    }
  }
  
  cache->cache_misses++;
  printf("JIT_CACHE: MISS for '%s' (hash %llx)\n", 
         fn_name, (unsigned long long)ir_hash);
  
  return NULL;
}

void nova_jit_cache_fn(NovaJITContext *ctx, const char *fn_name,
                         NovaHash ir_hash, void *code, size_t size) {
  JITCache *cache = get_global_cache();
  
  printf("JIT_CACHE: Caching '%s' (%zu bytes, hash %llx)\n",
         fn_name, size, (unsigned long long)ir_hash);
  
  // Add to memory cache
  add_entry(cache, ir_hash, fn_name, code, size);
  
  // Save to disk
  if (cache->persistence_enabled) {
    save_to_disk(ir_hash, code, size);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// CACHE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void nova_jit_cache_clear(void) {
  JITCache *cache = get_global_cache();
  
  for (size_t i = 0; i < cache->entry_count; i++) {
    free(cache->entries[i].machine_code);
    free(cache->entries[i].function_name);
  }
  
  cache->entry_count = 0;
  cache->total_code_size = 0;
  cache->cache_hits = 0;
  cache->cache_misses = 0;
}

void nova_jit_cache_print_stats(void) {
  JITCache *cache = get_global_cache();
  
  printf("\n=== JIT Cache Statistics ===\n");
  printf("Entries: %zu / %zu\n", cache->entry_count, cache->max_entries);
  printf("Total size: %zu KB / %zu MB\n",
         cache->total_code_size / 1024,
         cache->max_cache_size / (1024 * 1024));
  printf("Cache hits: %zu\n", cache->cache_hits);
  printf("Cache misses: %zu\n", cache->cache_misses);
  
  if (cache->cache_hits + cache->cache_misses > 0) {
    double hit_rate = (double)cache->cache_hits / 
                      (cache->cache_hits + cache->cache_misses) * 100.0;
    printf("Hit rate: %.2f%%\n", hit_rate);
  }
  printf("============================\n\n");
}
