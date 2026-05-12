/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_memory.h - Advanced Memory Management (Rust Ownership Benzeri)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_MEMORY_H
#define NOVA_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Ownership tracking (compile-time + runtime hybrid)
typedef enum {
  NOVA_OWNER_UNIQUE,   // Rust'ın Box gibi
  NOVA_OWNER_SHARED,   // Rust'ın Arc gibi
  NOVA_OWNER_BORROWED, // Rust'ın &T gibi
  NOVA_OWNER_STATIC,   // 'static lifetime
} NovaOwnership;

typedef struct {
  void *data;
  size_t size;
  NovaOwnership ownership;
  _Atomic uint32_t refcount; // For SHARED ownership
  void (*destructor)(void *);
} NovaBox;

// Smart pointer operations
NovaBox *nova_box_new(size_t size, void (*destructor)(void *));
NovaBox *nova_box_clone(NovaBox *box);
void *nova_box_borrow(NovaBox *box);
void nova_box_drop(NovaBox *box);

// Arena allocator (fast, deterministic)
typedef struct NovaArena NovaArena;

NovaArena *nova_arena_create(size_t initial_size);
void *nova_arena_alloc(NovaArena *arena, size_t size);
void nova_arena_reset(NovaArena *arena);
void nova_arena_destroy(NovaArena *arena);

// Pool allocator (object pooling)
typedef struct NovaPool NovaPool;

NovaPool *nova_pool_create(size_t object_size, size_t capacity);
void *nova_pool_alloc(NovaPool *pool);
void nova_pool_free(NovaPool *pool, void *ptr);
void nova_pool_destroy(NovaPool *pool);

// Stack-based memory (compile-time sized)
#define NOVA_STACK_ALLOC(type, count)                                        \
  ((type *)__builtin_alloca((count) * sizeof(type)))

// Memory safety checks (debug builds)
#ifdef NOVA_DEBUG
void nova_memory_check_bounds(const void *ptr, size_t size);
void nova_memory_check_alignment(const void *ptr, size_t alignment);
void nova_memory_poison(void *ptr, size_t size);
void nova_memory_unpoison(void *ptr, size_t size);
#else
#define nova_memory_check_bounds(ptr, size) ((void)0)
#define nova_memory_check_alignment(ptr, alignment) ((void)0)
#define nova_memory_poison(ptr, size) ((void)0)
#define nova_memory_unpoison(ptr, size) ((void)0)
#endif

// Lifetime analysis (compiler support needed)
typedef struct {
  const char *name;
  uint64_t birth_time;
  uint64_t death_time;
  bool escaped;
} NovaLifetime;

bool nova_lifetime_check(NovaLifetime *lifetime);

#endif // NOVA_MEMORY_H