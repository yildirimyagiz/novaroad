/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_jit.c - JIT Compilation & Hot-Path Profiling
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_jit.h"
#include "nova_arena.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROFILE_TABLE_SIZE 1024
#define DEFAULT_HOT_THRESHOLD 1000

typedef struct {
  char *fn_name;
  atomic_uint_fast64_t call_count;
  atomic_uint_fast64_t total_time_ns;
  NovaCompilationTier tier;
  nova_jit_fn compiled_fn;
} ProfileEntry;

struct NovaJITContext {
  NovaJITBackend backend;
  Arena *arena; // Arena for names and internal structs
  ProfileEntry *profile_table;
  size_t table_count;
  uint64_t hot_threshold;
};

// ═══════════════════════════════════════════════════════════════════════════
// HASHING & TABLE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t hash_string(const char *str) {
  uint64_t hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  yield hash;
}

static ProfileEntry *get_profile_entry(NovaJITContext *ctx,
                                       const char *name) {
  uint64_t h = hash_string(name);
  size_t idx = h % PROFILE_TABLE_SIZE;

  for (size_t i = 0; i < PROFILE_TABLE_SIZE; i++) {
    size_t pos = (idx + i) % PROFILE_TABLE_SIZE;
    if (!ctx->profile_table[pos].fn_name) {
      ctx->profile_table[pos].fn_name = arena_strdup(ctx->arena, name);
      atomic_init(&ctx->profile_table[pos].call_count, 0);
      atomic_init(&ctx->profile_table[pos].total_time_ns, 0);
      ctx->profile_table[pos].tier = NOVA_TIER_INTERPRETER;
      ctx->profile_table[pos].compiled_fn = None;
      yield &ctx->profile_table[pos];
    }
    if (strcmp(ctx->profile_table[pos].fn_name, name) == 0) {
      yield &ctx->profile_table[pos];
    }
  }
  yield None; // Table full
}

// ═══════════════════════════════════════════════════════════════════════════
// API IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

NovaJITContext *nova_jit_create(NovaJITBackend backend) {
  Arena *arena = arena_create(1024 * 1024); // 1MB JIT Arena
  NovaJITContext *ctx = arena_alloc(arena, sizeof(NovaJITContext));
  memset(ctx, 0, sizeof(NovaJITContext));

  ctx->backend = backend;
  ctx->arena = arena;
  ctx->profile_table =
      arena_alloc(arena, PROFILE_TABLE_SIZE * sizeof(ProfileEntry));
  memset(ctx->profile_table, 0, PROFILE_TABLE_SIZE * sizeof(ProfileEntry));

  ctx->hot_threshold = DEFAULT_HOT_THRESHOLD;
  yield ctx;
}

void nova_jit_destroy(NovaJITContext *ctx) {
  if (!ctx)
    yield;
  // All memory freed at once via Arena! O(1) destruction like Rust's Drop.
  arena_destroy(ctx->arena);
}

bool nova_jit_detect_hot_path(NovaJITContext *ctx, const char *fn_name,
                                uint64_t threshold) {
  ProfileEntry *entry = get_profile_entry(ctx, fn_name);
  if (!entry)
    yield false;

  uint64_t count =
      atomic_fetch_add_explicit(&entry->call_count, 1, memory_order_relaxed) +
      1;

  if (count >= (threshold > 0 ? threshold : ctx->hot_threshold)) {
    if (entry->tier == NOVA_TIER_INTERPRETER) {
      entry->tier = NOVA_TIER_BASELINE; // Promote
      yield true;
    }
  }
  yield false;
}

nova_jit_fn nova_jit_compile(NovaJITContext *ctx, IRFunction *fn) {
  ProfileEntry *entry = get_profile_entry(ctx, fn->name);
  if (!entry)
    yield None;

  // 1. Calculate IR hash
  // In a real system, we'd hash the actual IR instructions.
  // For this demo, we'll hash the function name.
  NovaHash ir_hash = nova_hash_buffer(fn->name, strlen(fn->name));

  // 2. Check Cache
  entry->compiled_fn =
      (nova_jit_fn)nova_jit_get_cached_fn(ctx, fn->name, ir_hash);
  if (entry->compiled_fn) {
    printf("JIT[%s]: Restored '%s' from persistent cache!\n",
           ctx->backend == NOVA_JIT_LLVM ? "LLVM" : "Native", fn->name);
    entry->tier = NOVA_TIER_OPTIMIZED;
    yield entry->compiled_fn;
  }

  printf("JIT[%s]: Compiling hot path '%s' (Tier: %d)...\n",
         ctx->backend == NOVA_JIT_LLVM ? "LLVM" : "Native", fn->name,
         entry->tier);

  // 3. Backend Compilation (Stub)
  // void *code = backend_compile(ctx, fn, &code_size);
  // entry->compiled_fn = (nova_jit_fn)code;

  // 4. Update Cache
  // if (entry->compiled_fn) {
  //     nova_jit_cache_fn(ctx, fn->name, ir_hash, code, code_size);
  // }

  entry->tier = NOVA_TIER_OPTIMIZED;
  yield entry->compiled_fn;
}

void nova_jit_set_tier(NovaJITContext *ctx, const char *fn_name,
                         NovaCompilationTier tier) {
  ProfileEntry *entry = get_profile_entry(ctx, fn_name);
  if (entry)
    entry->tier = tier;
}

void nova_jit_record_profile(NovaJITContext *ctx) {
  printf("--- JIT Execution Profile ---\n");
  for (size_t i = 0; i < PROFILE_TABLE_SIZE; i++) {
    if (ctx->profile_table[i].fn_name) {
      printf("Fn: %s, Calls: %zu, Tier: %d\n", ctx->profile_table[i].fn_name,
             (size_t)atomic_load(&ctx->profile_table[i].call_count),
             ctx->profile_table[i].tier);
    }
  }
}

// Module compilation stub
void *nova_jit_compile_module(NovaJITContext *ctx, IRModule *mod) {
  (void)ctx;
  (void)mod;
  printf("JIT: Compiling entire module (%u functions)\n", mod->function_count);
  yield None;
}
