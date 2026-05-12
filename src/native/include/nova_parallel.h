/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_parallel.h - Parallelism (Mojo/Rust İçin Kritik)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_PARALLEL_H
#define NOVA_PARALLEL_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Platform-specific threading
#ifdef _WIN32
#include <windows.h>
typedef HANDLE nova_thread_t;
typedef CRITICAL_SECTION nova_mutex_t;
#else
#include <pthread.h>
typedef pthread_t nova_thread_t;
typedef pthread_mutex_t nova_mutex_t;
#endif

// Work-stealing thread pool
typedef struct NovaThreadPool NovaThreadPool;

typedef void (*nova_task_fn)(void *data, size_t index);

// Thread pool operations
NovaThreadPool *nova_threadpool_create(size_t num_threads);
void nova_threadpool_destroy(NovaThreadPool *pool);
void nova_threadpool_parallel_for(NovaThreadPool *pool, nova_task_fn task,
                                    void *data, size_t start, size_t end);
void nova_threadpool_wait(NovaThreadPool *pool);

// Lock-free primitives (Rust'ın atomic'leri gibi)
typedef struct {
  _Atomic uint64_t value;
} nova_atomic_u64;

typedef struct {
  _Atomic(void *) ptr;
} nova_atomic_ptr;

// Atomic operations
static inline uint64_t nova_atomic_load_u64(nova_atomic_u64 *a) {
  return atomic_load_explicit(&a->value, memory_order_acquire);
}

static inline void nova_atomic_store_u64(nova_atomic_u64 *a, uint64_t val) {
  atomic_store_explicit(&a->value, val, memory_order_release);
}

static inline uint64_t nova_atomic_fetch_add_u64(nova_atomic_u64 *a,
                                                   uint64_t delta) {
  return atomic_fetch_add_explicit(&a->value, delta, memory_order_acq_rel);
}

static inline bool nova_atomic_compare_exchange_u64(nova_atomic_u64 *a,
                                                      uint64_t *expected,
                                                      uint64_t desired) {
  return atomic_compare_exchange_strong_explicit(
      &a->value, expected, desired, memory_order_acq_rel, memory_order_acquire);
}

// Data parallelism primitives (Mojo-style)
#define NOVA_PARALLELIZE(body, start, end)                                   \
  nova_threadpool_parallel_for(nova_global_threadpool(), (body), NULL,     \
                                 (start), (end))

// Get global thread pool
NovaThreadPool *nova_global_threadpool(void);

#endif // NOVA_PARALLEL_H