/**
 * @file thread.h
 * @brief Cross-platform Threading Abstraction
 */

#ifndef NOVA_THREAD_H
#define NOVA_THREAD_H

#include "../config/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Thread handle */
typedef struct nova_thread nova_thread_t;

/* Mutex handle */
typedef struct nova_mutex nova_mutex_t;

/* Condition variable handle */
typedef struct nova_cond nova_cond_t;

/* Thread function signature */
typedef void (*nova_thread_func_t)(void *arg);

/* ============================================================================
 * Thread Management
 * ========================================================================== */

/**
 * Create new thread
 */
nova_thread_t *nova_thread_create(nova_thread_func_t func, void *arg);

/**
 * Wait for thread to complete
 */
int nova_thread_join(nova_thread_t *thread);

/**
 * Detach thread (runs independently)
 */
int nova_thread_detach(nova_thread_t *thread);

/**
 * Destroy thread handle
 */
void nova_thread_destroy(nova_thread_t *thread);

/**
 * Get current thread ID
 */
size_t nova_thread_current_id(void);

/**
 * Sleep for specified milliseconds
 */
void nova_thread_sleep(unsigned int milliseconds);

/**
 * Yield current thread
 */
void nova_thread_yield(void);

/* ============================================================================
 * Mutex (Mutual Exclusion)
 * ========================================================================== */

/**
 * Create mutex
 */
nova_mutex_t *nova_mutex_create(void);

/**
 * Destroy mutex
 */
void nova_mutex_destroy(nova_mutex_t *mutex);

/**
 * Lock mutex
 */
int nova_mutex_lock(nova_mutex_t *mutex);

/**
 * Try to lock mutex (non-blocking)
 */
int nova_mutex_try_lock(nova_mutex_t *mutex);

/**
 * Unlock mutex
 */
int nova_mutex_unlock(nova_mutex_t *mutex);

/* ============================================================================
 * Condition Variables
 * ========================================================================== */

/**
 * Create condition variable
 */
nova_cond_t *nova_cond_create(void);

/**
 * Destroy condition variable
 */
void nova_cond_destroy(nova_cond_t *cond);

/**
 * Wait on condition variable
 */
int nova_cond_wait(nova_cond_t *cond, nova_mutex_t *mutex);

/**
 * Signal one waiting thread
 */
int nova_cond_signal(nova_cond_t *cond);

/**
 * Signal all waiting threads
 */
int nova_cond_broadcast(nova_cond_t *cond);

/**
 * Wait on condition with timeout (milliseconds)
 */
int nova_cond_timed_wait(nova_cond_t *cond, nova_mutex_t *mutex, unsigned int timeout_ms);

/* ============================================================================
 * Thread Pool
 * ========================================================================== */

typedef struct nova_thread_pool nova_thread_pool_t;

/**
 * Create thread pool
 */
nova_thread_pool_t *nova_thread_pool_create(size_t num_threads);

/**
 * Destroy thread pool
 */
void nova_thread_pool_destroy(nova_thread_pool_t *pool);

/**
 * Submit task to thread pool
 */
int nova_thread_pool_submit(nova_thread_pool_t *pool, nova_thread_func_t func, void *arg);

/**
 * Wait for all tasks to complete
 */
int nova_thread_pool_wait(nova_thread_pool_t *pool);

/* ============================================================================
 * Atomic Operations
 * ========================================================================== */

/**
 * Atomic integer type
 */
typedef struct {
#if defined(NOVA_PLATFORM_POSIX)
    volatile int value;
#elif defined(NOVA_PLATFORM_WINDOWS)
    volatile long value;
#endif
} nova_atomic_int_t;

/**
 * Initialize atomic integer
 */
void nova_atomic_init(nova_atomic_int_t *atomic, int value);

/**
 * Atomic load
 */
int nova_atomic_load(nova_atomic_int_t *atomic);

/**
 * Atomic store
 */
void nova_atomic_store(nova_atomic_int_t *atomic, int value);

/**
 * Atomic compare and exchange
 */
bool nova_atomic_compare_exchange(nova_atomic_int_t *atomic, int expected, int desired);

/**
 * Atomic increment
 */
int nova_atomic_increment(nova_atomic_int_t *atomic);

/**
 * Atomic decrement
 */
int nova_atomic_decrement(nova_atomic_int_t *atomic);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_THREAD_H */
