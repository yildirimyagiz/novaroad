/**
 * @file mutex.c
 * @brief Advanced mutex implementation with recursive locking and statistics
 */

#include "runtime/runtime.h"
#include "runtime/thread.h"
#include "std/alloc.h"
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MUTEX_MAX_RECURSION 32

// Waiter queue for priority inheritance
typedef struct waiter_entry {
    pthread_t thread;
    uint64_t wait_start_time;
    int priority;
    struct waiter_entry *next;
} waiter_entry_t;

struct nova_mutex {
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    // Recursive locking support
    pthread_t owner;
    int recursion_count;
    int max_recursion_depth;

    // Type and attributes
    nova_mutex_type_t type;
    int protocol; // Priority inheritance protocol

    // Statistics
    atomic_size_t lock_count;
    atomic_size_t unlock_count;
    atomic_size_t wait_count;
    atomic_size_t contention_count;
    uint64_t total_wait_time_ns;
    uint64_t max_wait_time_ns;
    uint64_t creation_time;

    // Wait queue for advanced features
    waiter_entry_t *wait_queue;
    pthread_mutex_t queue_mutex;
};

struct nova_rwlock {
    pthread_rwlock_t rwlock;
};

nova_mutex_t *nova_mutex_create(void)
{
    return nova_mutex_create_with_type(NOVA_MUTEX_NORMAL);
}

nova_mutex_t *nova_mutex_create_with_type(nova_mutex_type_t type)
{
    nova_mutex_t *mutex = nova_alloc(sizeof(nova_mutex_t));
    if (!mutex)
        return NULL;

    memset(mutex, 0, sizeof(nova_mutex_t));

    // Set up mutex attributes
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);

    switch (type) {
    case NOVA_MUTEX_NORMAL:
        mutex->type = NOVA_MUTEX_NORMAL;
        break;
    case NOVA_MUTEX_RECURSIVE:
        mutex->type = NOVA_MUTEX_RECURSIVE;
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        break;
    case NOVA_MUTEX_ERROR_CHECK:
        mutex->type = NOVA_MUTEX_ERROR_CHECK;
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        break;
    }

    // Initialize synchronization primitives
    if (pthread_mutex_init(&mutex->mutex, &attr) != 0) {
        nova_free(mutex);
        pthread_mutexattr_destroy(&attr);
        pthread_condattr_destroy(&cond_attr);
        return NULL;
    }

    if (pthread_cond_init(&mutex->cond, &cond_attr) != 0) {
        pthread_mutex_destroy(&mutex->mutex);
        nova_free(mutex);
        pthread_mutexattr_destroy(&attr);
        pthread_condattr_destroy(&cond_attr);
        return NULL;
    }

    if (pthread_mutex_init(&mutex->queue_mutex, NULL) != 0) {
        pthread_mutex_destroy(&mutex->mutex);
        pthread_cond_destroy(&mutex->cond);
        nova_free(mutex);
        pthread_mutexattr_destroy(&attr);
        pthread_condattr_destroy(&cond_attr);
        return NULL;
    }

    // Clean up attributes
    pthread_mutexattr_destroy(&attr);
    pthread_condattr_destroy(&cond_attr);

    // Initialize fields
    mutex->owner = 0;
    mutex->recursion_count = 0;
    mutex->max_recursion_depth = 0;
    mutex->protocol = 0;
    mutex->creation_time = nova_get_time_ns();

    atomic_init(&mutex->lock_count, 0);
    atomic_init(&mutex->unlock_count, 0);
    atomic_init(&mutex->wait_count, 0);
    atomic_init(&mutex->contention_count, 0);
    mutex->total_wait_time_ns = 0;
    mutex->max_wait_time_ns = 0;

    mutex->wait_queue = NULL;

    return mutex;
}

void nova_mutex_destroy(nova_mutex_t *mutex)
{
    if (!mutex)
        return;

    // Clean up wait queue
    pthread_mutex_lock(&mutex->queue_mutex);
    waiter_entry_t *entry = mutex->wait_queue;
    while (entry) {
        waiter_entry_t *next = entry->next;
        nova_free(entry);
        entry = next;
    }
    pthread_mutex_unlock(&mutex->queue_mutex);

    // Destroy synchronization primitives
    pthread_mutex_destroy(&mutex->queue_mutex);
    pthread_cond_destroy(&mutex->cond);
    pthread_mutex_destroy(&mutex->mutex);

    nova_free(mutex);
}

int nova_mutex_lock(nova_mutex_t *mutex)
{
    if (!mutex)
        return -1;

    int result = 0;
    uint64_t wait_start = nova_get_time_ns();

    // Handle recursive locking
    if (mutex->type == NOVA_MUTEX_RECURSIVE) {
        if (pthread_equal(mutex->owner, pthread_self())) {
            mutex->recursion_count++;
            if (mutex->recursion_count > mutex->max_recursion_depth) {
                mutex->max_recursion_depth = mutex->recursion_count;
            }
            atomic_fetch_add(&mutex->lock_count, 1);
            return 0;
        }
    }

    // Add to wait queue
    waiter_entry_t waiter = {.thread = pthread_self(),
                             .wait_start_time = wait_start,
                             .priority = 0, // Could be set based on thread priority
                             .next = NULL};

    pthread_mutex_lock(&mutex->queue_mutex);
    waiter.next = mutex->wait_queue;
    mutex->wait_queue = &waiter;
    pthread_mutex_unlock(&mutex->queue_mutex);

    // Attempt to acquire lock
    result = pthread_mutex_lock(&mutex->mutex);

    if (result == 0) {
        uint64_t wait_end = nova_get_time_ns();
        uint64_t wait_time = wait_end - wait_start;

        // Update statistics
        atomic_fetch_add(&mutex->lock_count, 1);
        if (wait_time > 0) {
            atomic_fetch_add(&mutex->wait_count, 1);
            atomic_fetch_add(&mutex->contention_count, 1);
            mutex->total_wait_time_ns += wait_time;
            if (wait_time > mutex->max_wait_time_ns) {
                mutex->max_wait_time_ns = wait_time;
            }
        }

        // Set ownership for recursive mutex
        if (mutex->type == NOVA_MUTEX_RECURSIVE) {
            mutex->owner = pthread_self();
            mutex->recursion_count = 1;
        }
    }

    // Remove from wait queue
    pthread_mutex_lock(&mutex->queue_mutex);
    waiter_entry_t **entry = &mutex->wait_queue;
    while (*entry) {
        if (*entry == &waiter) {
            *entry = waiter.next;
            break;
        }
        entry = &(*entry)->next;
    }
    pthread_mutex_unlock(&mutex->queue_mutex);

    return result;
}

int nova_mutex_try_lock(nova_mutex_t *mutex)
{
    if (!mutex)
        return -1;

    // Handle recursive locking
    if (mutex->type == NOVA_MUTEX_RECURSIVE) {
        if (pthread_equal(mutex->owner, pthread_self())) {
            mutex->recursion_count++;
            if (mutex->recursion_count > mutex->max_recursion_depth) {
                mutex->max_recursion_depth = mutex->recursion_count;
            }
            atomic_fetch_add(&mutex->lock_count, 1);
            return 0;
        }
    }

    int result = pthread_mutex_trylock(&mutex->mutex);

    if (result == 0) {
        atomic_fetch_add(&mutex->lock_count, 1);

        // Set ownership for recursive mutex
        if (mutex->type == NOVA_MUTEX_RECURSIVE) {
            mutex->owner = pthread_self();
            mutex->recursion_count = 1;
        }
    } else if (result == EBUSY) {
        atomic_fetch_add(&mutex->contention_count, 1);
    }

    return result;
}

int nova_mutex_timed_lock(nova_mutex_t *mutex, uint64_t timeout_ns)
{
    if (!mutex)
        return -1;

    // Handle recursive locking
    if (mutex->type == NOVA_MUTEX_RECURSIVE) {
        if (pthread_equal(mutex->owner, pthread_self())) {
            mutex->recursion_count++;
            if (mutex->recursion_count > mutex->max_recursion_depth) {
                mutex->max_recursion_depth = mutex->recursion_count;
            }
            atomic_fetch_add(&mutex->lock_count, 1);
            return 0;
        }
    }

    uint64_t wait_start = nova_get_time_ns();

    // Add to wait queue
    waiter_entry_t waiter = {
        .thread = pthread_self(), .wait_start_time = wait_start, .priority = 0, .next = NULL};

    pthread_mutex_lock(&mutex->queue_mutex);
    waiter.next = mutex->wait_queue;
    mutex->wait_queue = &waiter;
    pthread_mutex_unlock(&mutex->queue_mutex);

    int result = ETIMEDOUT;
    while (nova_get_time_ns() - wait_start < timeout_ns) {
        result = pthread_mutex_trylock(&mutex->mutex);
        if (result == 0)
            break;
        if (result != EBUSY)
            break;
        usleep(100); // Poll every 100us
    }

    if (result == 0) {
        uint64_t wait_end = nova_get_time_ns();
        uint64_t wait_time = wait_end - wait_start;

        // Update statistics
        atomic_fetch_add(&mutex->lock_count, 1);
        if (wait_time > 0) {
            atomic_fetch_add(&mutex->wait_count, 1);
            atomic_fetch_add(&mutex->contention_count, 1);
            mutex->total_wait_time_ns += wait_time;
            if (wait_time > mutex->max_wait_time_ns) {
                mutex->max_wait_time_ns = wait_time;
            }
        }

        // Set ownership for recursive mutex
        if (mutex->type == NOVA_MUTEX_RECURSIVE) {
            mutex->owner = pthread_self();
            mutex->recursion_count = 1;
        }
    }

    // Remove from wait queue
    pthread_mutex_lock(&mutex->queue_mutex);
    waiter_entry_t **entry = &mutex->wait_queue;
    while (*entry) {
        if (*entry == &waiter) {
            *entry = waiter.next;
            break;
        }
        entry = &(*entry)->next;
    }
    pthread_mutex_unlock(&mutex->queue_mutex);

    return result;
}

int nova_mutex_unlock(nova_mutex_t *mutex)
{
    if (!mutex)
        return -1;

    // Handle recursive locking
    if (mutex->type == NOVA_MUTEX_RECURSIVE) {
        if (pthread_equal(mutex->owner, pthread_self())) {
            mutex->recursion_count--;
            if (mutex->recursion_count > 0) {
                atomic_fetch_add(&mutex->unlock_count, 1);
                return 0; // Still holding the lock
            }
        }
    }

    int result = pthread_mutex_unlock(&mutex->mutex);

    if (result == 0) {
        atomic_fetch_add(&mutex->unlock_count, 1);

        // Clear ownership for recursive mutex
        if (mutex->type == NOVA_MUTEX_RECURSIVE) {
            mutex->owner = 0;
            mutex->recursion_count = 0;
        }
    }

    return result;
}

int nova_mutex_is_locked(nova_mutex_t *mutex)
{
    if (!mutex)
        return 0;

    // Try to acquire lock without blocking
    int result = pthread_mutex_trylock(&mutex->mutex);
    if (result == 0) {
        // We got the lock, so it wasn't locked
        pthread_mutex_unlock(&mutex->mutex);
        return 0;
    } else if (result == EBUSY) {
        // Lock is held by another thread
        return 1;
    }

    // Error condition
    return -1;
}

pthread_t nova_mutex_get_owner(nova_mutex_t *mutex)
{
    if (!mutex || mutex->type != NOVA_MUTEX_RECURSIVE)
        return 0;
    return mutex->owner;
}

int nova_mutex_get_recursion_count(nova_mutex_t *mutex)
{
    if (!mutex || mutex->type != NOVA_MUTEX_RECURSIVE)
        return 0;
    return mutex->recursion_count;
}

nova_mutex_stats_t nova_mutex_stats(nova_mutex_t *mutex)
{
    nova_mutex_stats_t stats = {0};

    if (!mutex)
        return stats;

    stats.lock_count = atomic_load(&mutex->lock_count);
    stats.unlock_count = atomic_load(&mutex->unlock_count);
    stats.wait_count = atomic_load(&mutex->wait_count);
    stats.contention_count = atomic_load(&mutex->contention_count);
    stats.total_wait_time_ns = mutex->total_wait_time_ns;
    stats.max_wait_time_ns = mutex->max_wait_time_ns;
    stats.creation_time = mutex->creation_time;

    stats.is_recursive = (mutex->type == NOVA_MUTEX_RECURSIVE);
    stats.current_owner = mutex->owner;
    stats.recursion_count = mutex->recursion_count;
    stats.max_recursion_depth = mutex->max_recursion_depth;

    // Calculate averages
    if (stats.wait_count > 0) {
        stats.avg_wait_time_ns = stats.total_wait_time_ns / stats.wait_count;
    }

    if (stats.lock_count > 0) {
        stats.contention_rate = (double) stats.contention_count / (double) stats.lock_count * 100.0;
    }

    return stats;
}

nova_rwlock_t *nova_rwlock_create(void)
{
    nova_rwlock_t *rwlock = nova_alloc(sizeof(nova_rwlock_t));
    if (!rwlock)
        return NULL;

    if (pthread_rwlock_init(&rwlock->rwlock, NULL) != 0) {
        nova_free(rwlock);
        return NULL;
    }

    return rwlock;
}

void nova_rwlock_destroy(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return;

    pthread_rwlock_destroy(&rwlock->rwlock);
    nova_free(rwlock);
}

int nova_rwlock_read_lock(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return -1;
    return pthread_rwlock_rdlock(&rwlock->rwlock);
}

int nova_rwlock_read_unlock(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return -1;
    return pthread_rwlock_unlock(&rwlock->rwlock);
}

int nova_rwlock_write_lock(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return -1;
    return pthread_rwlock_wrlock(&rwlock->rwlock);
}

int nova_rwlock_write_unlock(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return -1;
    return pthread_rwlock_unlock(&rwlock->rwlock);
}

int nova_rwlock_try_read_lock(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return -1;
    return pthread_rwlock_tryrdlock(&rwlock->rwlock);
}

int nova_rwlock_try_write_lock(nova_rwlock_t *rwlock)
{
    if (!rwlock)
        return -1;
    return pthread_rwlock_trywrlock(&rwlock->rwlock);
}
