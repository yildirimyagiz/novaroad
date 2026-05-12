/**
 * @file thread.h
 * @brief Threading primitives (threads, mutexes, channels, thread-local storage)
 *
 * Nova provides:
 * - OS threads (1:1 threading)
 * - Synchronization primitives (mutex, rwlock, condvar, semaphore)
 * - Thread-safe channels (Go-style CSP)
 * - Thread pools
 * - Thread-local storage
 */

#ifndef NOVA_THREAD_H
#define NOVA_THREAD_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Threads
 * ======================================================================== */

typedef struct nova_thread nova_thread_t;
typedef void *(*nova_thread_func_t)(void *arg);

/**
 * @brief Create and start thread
 * @param func Thread entry point
 * @param arg Argument passed to function
 * @return Thread handle, or NULL on failure
 */
nova_thread_t *nova_thread_create(nova_thread_func_t func, void *arg);

/**
 * @brief Create thread with custom stack size
 * @param func Thread entry point
 * @param arg Argument
 * @param stack_size Stack size in bytes
 * @return Thread handle, or NULL on failure
 */
nova_thread_t *nova_thread_create_with_stack(nova_thread_func_t func, void *arg, size_t stack_size);

/**
 * @brief Wait for thread to finish
 * @param thread Thread handle
 * @return Thread return value
 */
void *nova_thread_join(nova_thread_t *thread);

/**
 * @brief Detach thread (runs independently)
 * @param thread Thread handle
 */
void nova_thread_detach(nova_thread_t *thread);

/**
 * @brief Get current thread handle
 * @return Current thread handle
 */
nova_thread_t *nova_thread_current(void);

/**
 * @brief Get thread ID
 * @param thread Thread handle
 * @return Thread ID
 */
uint64_t nova_thread_id(nova_thread_t *thread);

/**
 * @brief Yield CPU to other threads
 */
void nova_thread_yield(void);

/**
 * @brief Sleep current thread
 * @param ms Milliseconds to sleep
 */
void nova_thread_sleep(uint32_t ms);

/* ========================================================================
 * Mutex
 * ======================================================================== */

typedef struct nova_mutex nova_mutex_t;

typedef enum { NOVA_MUTEX_NORMAL, NOVA_MUTEX_RECURSIVE, NOVA_MUTEX_ERROR_CHECK } nova_mutex_type_t;

typedef struct {
    size_t lock_count;
    size_t unlock_count;
    size_t wait_count;
    size_t contention_count;
    uint64_t total_wait_time_ns;
    uint64_t max_wait_time_ns;
    uint64_t avg_wait_time_ns;
    uint64_t creation_time;
    bool is_recursive;
    pthread_t current_owner;
    int recursion_count;
    int max_recursion_depth;
    double contention_rate;
} nova_mutex_stats_t;

/**
 * @brief Create mutex
 * @return Mutex handle, or NULL on failure
 */
nova_mutex_t *nova_mutex_create(void);

/**
 * @brief Create mutex with type
 * @param type Mutex type
 * @return Mutex handle, or NULL on failure
 */
nova_mutex_t *nova_mutex_create_with_type(nova_mutex_type_t type);

/**
 * @brief Lock mutex
 * @param mutex Mutex handle
 * @return 0 on success, -1 on failure
 */
int nova_mutex_lock(nova_mutex_t *mutex);

/**
 * @brief Try to lock mutex
 * @param mutex Mutex handle
 * @return 0 if locked, EBUSY if already locked
 */
int nova_mutex_try_lock(nova_mutex_t *mutex);

/**
 * @brief Timed lock
 * @param mutex Mutex handle
 * @param timeout_ns Timeout in nanoseconds
 * @return 0 on success, ETIMEDOUT on timeout
 */
int nova_mutex_timed_lock(nova_mutex_t *mutex, uint64_t timeout_ns);

/**
 * @brief Unlock mutex
 * @param mutex Mutex handle
 * @return 0 on success, -1 on failure
 */
int nova_mutex_unlock(nova_mutex_t *mutex);

/**
 * @brief Destroy mutex
 * @param mutex Mutex handle
 */
void nova_mutex_destroy(nova_mutex_t *mutex);

/**
 * @brief Get mutex statistics
 * @param mutex Mutex handle
 * @return Mutex statistics
 */
nova_mutex_stats_t nova_mutex_stats(nova_mutex_t *mutex);

/* ========================================================================
 * Read-Write Lock
 * ======================================================================== */

typedef struct nova_rwlock nova_rwlock_t;

/**
 * @brief Create read-write lock
 * @return RWLock handle, or NULL on failure
 */
nova_rwlock_t *nova_rwlock_create(void);

/**
 * @brief Acquire read lock
 * @param rwlock RWLock handle
 * @return 0 on success
 */
int nova_rwlock_read_lock(nova_rwlock_t *rwlock);

/**
 * @brief Acquire write lock
 * @param rwlock RWLock handle
 * @return 0 on success
 */
int nova_rwlock_write_lock(nova_rwlock_t *rwlock);

/**
 * @brief Release read lock
 * @param rwlock RWLock handle
 * @return 0 on success
 */
int nova_rwlock_read_unlock(nova_rwlock_t *rwlock);

/**
 * @brief Release write lock
 * @param rwlock RWLock handle
 * @return 0 on success
 */
int nova_rwlock_write_unlock(nova_rwlock_t *rwlock);

/**
 * @brief Try read lock
 * @param rwlock RWLock handle
 * @return 0 on success
 */
int nova_rwlock_try_read_lock(nova_rwlock_t *rwlock);

/**
 * @brief Try write lock
 * @param rwlock RWLock handle
 * @return 0 on success
 */
int nova_rwlock_try_write_lock(nova_rwlock_t *rwlock);

/**
 * @brief Destroy RWLock
 * @param rwlock RWLock handle
 */
void nova_rwlock_destroy(nova_rwlock_t *rwlock);

/* ========================================================================
 * Condition Variable
 * ======================================================================== */

typedef struct nova_condvar nova_condvar_t;

/**
 * @brief Create condition variable
 * @return CondVar handle, or NULL on failure
 */
nova_condvar_t *nova_condvar_create(void);

/**
 * @brief Wait on condition variable
 * @param condvar CondVar handle
 * @param mutex Associated mutex
 */
void nova_condvar_wait(nova_condvar_t *condvar, nova_mutex_t *mutex);

/**
 * @brief Wait with timeout
 * @param condvar CondVar handle
 * @param mutex Associated mutex
 * @param timeout_ms Timeout in milliseconds
 * @return true if signaled, false if timeout
 */
bool nova_condvar_wait_timeout(nova_condvar_t *condvar, nova_mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief Signal one waiting thread
 * @param condvar CondVar handle
 */
void nova_condvar_signal(nova_condvar_t *condvar);

/**
 * @brief Signal all waiting threads
 * @param condvar CondVar handle
 */
void nova_condvar_broadcast(nova_condvar_t *condvar);

/**
 * @brief Destroy condition variable
 * @param condvar CondVar handle
 */
void nova_condvar_destroy(nova_condvar_t *condvar);

/* ========================================================================
 * Semaphore
 * ======================================================================== */

typedef struct nova_semaphore nova_semaphore_t;

/**
 * @brief Create semaphore
 * @param initial_count Initial count
 * @return Semaphore handle, or NULL on failure
 */
nova_semaphore_t *nova_semaphore_create(uint32_t initial_count);

/**
 * @brief Wait (decrement) semaphore
 * @param sem Semaphore handle
 */
void nova_semaphore_wait(nova_semaphore_t *sem);

/**
 * @brief Post (increment) semaphore
 * @param sem Semaphore handle
 */
void nova_semaphore_post(nova_semaphore_t *sem);

/**
 * @brief Try wait (non-blocking)
 * @param sem Semaphore handle
 * @return true if successful, false otherwise
 */
bool nova_semaphore_try_wait(nova_semaphore_t *sem);

/**
 * @brief Destroy semaphore
 * @param sem Semaphore handle
 */
void nova_semaphore_destroy(nova_semaphore_t *sem);

/* ========================================================================
 * Channels (Go-style CSP)
 * ======================================================================== */

#ifndef NOVA_CHANNEL_TYPE_DEFINED
#define NOVA_CHANNEL_TYPE_DEFINED
typedef struct nova_channel nova_channel_t;
#endif

typedef enum { NOVA_SELECT_SEND, NOVA_SELECT_RECV, NOVA_SELECT_DEFAULT } nova_select_dir_t;

typedef struct {
    nova_channel_t *channel;
    nova_select_dir_t dir;
    void *data;
    int result;
} nova_select_case_t;

typedef struct {
    size_t capacity;
    size_t size;
    bool is_closed;
    size_t sends;
    size_t recvs;
    size_t blocks;
} nova_channel_stats_t;

/**
 * @brief Create channel
 * @param capacity Channel capacity (0 = unbuffered)
 * @return Channel handle, or NULL on failure
 */
nova_channel_t *nova_channel_create(size_t capacity);

/**
 * @brief Close channel
 * @param ch Channel handle
 */
void nova_channel_close(nova_channel_t *ch);

/**
 * @brief Destroy channel
 * @param ch Channel handle
 */
void nova_channel_destroy(nova_channel_t *ch);

/**
 * @brief Send value to channel (blocking)
 * @param ch Channel handle
 * @param data Value to send
 * @return 0 on success, -1 on failure
 */
int nova_channel_send(nova_channel_t *ch, void *data);

/**
 * @brief Receive value from channel (blocking)
 * @param ch Channel handle
 * @param data Pointer to store received value
 * @return 0 on success, -1 on failure
 */
int nova_channel_recv(nova_channel_t *ch, void **data);

/**
 * @brief Try to send value (non-blocking)
 * @param ch Channel handle
 * @param data Value to send
 * @return 1 on success, 0 if full, -1 on failure
 */
int nova_channel_try_send(nova_channel_t *ch, void *data);

/**
 * @brief Try to receive value (non-blocking)
 * @param ch Channel handle
 * @param data Pointer to store received value
 * @return 1 on success, 0 if empty, -1 on failure
 */
int nova_channel_try_recv(nova_channel_t *ch, void **data);

/**
 * @brief Get number of items in channel
 * @param ch Channel handle
 * @return Number of items
 */
size_t nova_channel_len(nova_channel_t *ch);

/**
 * @brief Get channel capacity
 * @param ch Channel handle
 * @return Capacity
 */
size_t nova_channel_cap(nova_channel_t *ch);

/**
 * @brief Check if channel is closed
 * @param ch Channel handle
 * @return 1 if closed, 0 if open
 */
int nova_channel_is_closed(nova_channel_t *ch);

/**
 * @brief Get channel statistics
 * @param ch Channel handle
 * @return Channel statistics
 */
nova_channel_stats_t nova_channel_stats(nova_channel_t *ch);

/**
 * @brief Select from multiple channel operations
 * @param cases Array of select cases
 * @param count Number of cases
 * @param selected_index Pointer to store index of selected case
 * @return 0 on success, -1 on failure
 */
int nova_channel_select(nova_select_case_t *cases, int count, int *selected_index);

/* ========================================================================
 * Thread Pool
 * ======================================================================== */

typedef struct nova_thread_pool nova_thread_pool_t;
typedef void (*nova_task_func_t)(void *arg);
typedef nova_task_func_t nova_task_fn_t;

typedef struct {
    size_t num_threads;
    size_t active_tasks;
    size_t pending_tasks;
    size_t total_tasks;
    size_t completed_tasks;
    size_t stolen_tasks;
} nova_thread_pool_stats_t;

/**
 * @brief Create thread pool
 * @param num_threads Number of worker threads (0 for CPU count)
 * @return Thread pool handle, or NULL on failure
 */
nova_thread_pool_t *nova_thread_pool_create(size_t num_threads);

/**
 * @brief Submit task to pool
 * @param pool Thread pool handle
 * @param func Task function
 * @param arg Task argument
 * @return 0 on success, -1 on failure
 */
int nova_thread_pool_submit(nova_thread_pool_t *pool, nova_task_func_t func, void *arg);

/**
 * @brief Wait for all tasks to complete
 * @param pool Thread pool handle
 */
void nova_thread_pool_wait(nova_thread_pool_t *pool);

/**
 * @brief Shutdown thread pool
 * @param pool Thread pool handle
 */
void nova_thread_pool_shutdown(nova_thread_pool_t *pool);

/**
 * @brief Destroy thread pool
 * @param pool Thread pool handle
 */
void nova_thread_pool_destroy(nova_thread_pool_t *pool);

/**
 * @brief Get number of active tasks
 * @param pool Thread pool handle
 * @return Active task count
 */
size_t nova_thread_pool_active_tasks(nova_thread_pool_t *pool);

/**
 * @brief Get number of pending tasks
 * @param pool Thread pool handle
 * @return Pending task count
 */
size_t nova_thread_pool_pending_tasks(nova_thread_pool_t *pool);

/**
 * @brief Get thread pool statistics
 * @param pool Thread pool handle
 * @param stats Output: statistics structure
 */
void nova_thread_pool_stats(nova_thread_pool_t *pool, nova_thread_pool_stats_t *stats);

/* ========================================================================
 * Thread-Local Storage
 * ======================================================================== */

typedef uint32_t nova_tls_key_t;

/**
 * @brief Create TLS key
 * @param destructor Optional destructor function
 * @return TLS key
 */
nova_tls_key_t nova_tls_create(void (*destructor)(void *));

/**
 * @brief Set TLS value
 * @param key TLS key
 * @param value Value to store
 */
void nova_tls_set(nova_tls_key_t key, void *value);

/**
 * @brief Get TLS value
 * @param key TLS key
 * @return Stored value, or NULL if not set
 */
void *nova_tls_get(nova_tls_key_t key);

/**
 * @brief Delete TLS key
 * @param key TLS key
 */
void nova_tls_delete(nova_tls_key_t key);

/* ========================================================================
 * Atomic Operations
 * ======================================================================== */

/**
 * @brief Get number of CPUs
 * @return Number of logical CPUs
 */
size_t nova_thread_cpu_count(void);

/**
 * @brief Set thread affinity
 * @param thread Thread handle
 * @param cpu_id CPU ID
 * @return 0 on success, -1 on failure
 */
int nova_thread_set_affinity(nova_thread_t *thread, size_t cpu_id);

/**
 * @brief Set thread priority
 * @param thread Thread handle
 * @param priority Priority value (-20 to 19, lower is higher priority)
 * @return 0 on success, -1 on failure
 */
int nova_thread_set_priority(nova_thread_t *thread, int priority);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_THREAD_H */
