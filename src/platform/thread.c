/**
 * @file thread.c
 * @brief Cross-platform Threading implementation
 */

#include "platform/thread.h"
#include "std/alloc.h"
#include <stdlib.h>

#if defined(NOVA_PLATFORM_POSIX)
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#elif defined(NOVA_PLATFORM_WINDOWS)
#include <windows.h>
#include <process.h>
#endif

/* Internal structures */
struct nova_thread {
#if defined(NOVA_PLATFORM_POSIX)
    pthread_t thread;
#elif defined(NOVA_PLATFORM_WINDOWS)
    HANDLE handle;
    unsigned int thread_id;
#endif
    bool detached;
};

struct nova_mutex {
#if defined(NOVA_PLATFORM_POSIX)
    pthread_mutex_t mutex;
#elif defined(NOVA_PLATFORM_WINDOWS)
    CRITICAL_SECTION cs;
#endif
};

struct nova_cond {
#if defined(NOVA_PLATFORM_POSIX)
    pthread_cond_t cond;
#elif defined(NOVA_PLATFORM_WINDOWS)
    CONDITION_VARIABLE cond;
#endif
};

/* Thread pool structures */
typedef struct {
    nova_thread_func_t func;
    void *arg;
} nova_thread_task_t;

struct nova_thread_pool {
    nova_thread_t **threads;
    size_t num_threads;
    nova_mutex_t *mutex;
    nova_cond_t *cond;
    nova_thread_task_t *tasks;
    size_t task_capacity;
    size_t task_count;
    size_t task_head;
    size_t task_tail;
    bool shutdown;
};

/* ============================================================================
 * Thread Management
 * ========================================================================== */

#if defined(NOVA_PLATFORM_POSIX)
static void *thread_wrapper(void *arg) {
    nova_thread_task_t *task = (nova_thread_task_t *)arg;
    task->func(task->arg);
    nova_free(task);
    return NULL;
}
#endif

nova_thread_t *nova_thread_create(nova_thread_func_t func, void *arg) {
    if (!func) return NULL;

    nova_thread_t *thread = nova_alloc(sizeof(nova_thread_t));
    if (!thread) return NULL;

    thread->detached = false;

#if defined(NOVA_PLATFORM_POSIX)
    nova_thread_task_t *task = nova_alloc(sizeof(nova_thread_task_t));
    if (!task) {
        nova_free(thread);
        return NULL;
    }
    task->func = func;
    task->arg = arg;

    if (pthread_create(&thread->thread, NULL, thread_wrapper, task) != 0) {
        nova_free(task);
        nova_free(thread);
        return NULL;
    }
#elif defined(NOVA_PLATFORM_WINDOWS)
    // Windows implementation would go here
    nova_free(thread);
    return NULL; // Not implemented yet
#endif

    return thread;
}

int nova_thread_join(nova_thread_t *thread) {
    if (!thread || thread->detached) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    void *result;
    if (pthread_join(thread->thread, &result) != 0) {
        return -1;
    }
#endif

    return 0;
}

int nova_thread_detach(nova_thread_t *thread) {
    if (!thread || thread->detached) return -1;

    thread->detached = true;

#if defined(NOVA_PLATFORM_POSIX)
    if (pthread_detach(thread->thread) != 0) {
        return -1;
    }
#endif

    return 0;
}

void nova_thread_destroy(nova_thread_t *thread) {
    if (thread) {
        if (!thread->detached) {
            nova_thread_join(thread);
        }
        nova_free(thread);
    }
}

size_t nova_thread_current_id(void) {
#if defined(NOVA_PLATFORM_POSIX)
    return (size_t)pthread_self();
#elif defined(NOVA_PLATFORM_WINDOWS)
    return (size_t)GetCurrentThreadId();
#else
    return 0;
#endif
}

void nova_thread_sleep(unsigned int milliseconds) {
#if defined(NOVA_PLATFORM_POSIX)
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#elif defined(NOVA_PLATFORM_WINDOWS)
    Sleep(milliseconds);
#endif
}

void nova_thread_yield(void) {
#if defined(NOVA_PLATFORM_POSIX)
    sched_yield();
#elif defined(NOVA_PLATFORM_WINDOWS)
    SwitchToThread();
#endif
}

/* ============================================================================
 * Mutex Implementation
 * ========================================================================== */

nova_mutex_t *nova_mutex_create(void) {
    nova_mutex_t *mutex = nova_alloc(sizeof(nova_mutex_t));
    if (!mutex) return NULL;

#if defined(NOVA_PLATFORM_POSIX)
    if (pthread_mutex_init(&mutex->mutex, NULL) != 0) {
        nova_free(mutex);
        return NULL;
    }
#elif defined(NOVA_PLATFORM_WINDOWS)
    InitializeCriticalSection(&mutex->cs);
#endif

    return mutex;
}

void nova_mutex_destroy(nova_mutex_t *mutex) {
    if (!mutex) return;

#if defined(NOVA_PLATFORM_POSIX)
    pthread_mutex_destroy(&mutex->mutex);
#elif defined(NOVA_PLATFORM_WINDOWS)
    DeleteCriticalSection(&mutex->cs);
#endif

    nova_free(mutex);
}

int nova_mutex_lock(nova_mutex_t *mutex) {
    if (!mutex) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return pthread_mutex_lock(&mutex->mutex) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    EnterCriticalSection(&mutex->cs);
    return 0;
#endif
}

int nova_mutex_try_lock(nova_mutex_t *mutex) {
    if (!mutex) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return pthread_mutex_trylock(&mutex->mutex) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return TryEnterCriticalSection(&mutex->cs) ? 0 : -1;
#endif
}

int nova_mutex_unlock(nova_mutex_t *mutex) {
    if (!mutex) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return pthread_mutex_unlock(&mutex->mutex) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    LeaveCriticalSection(&mutex->cs);
    return 0;
#endif
}

/* ============================================================================
 * Condition Variable Implementation
 * ========================================================================== */

nova_cond_t *nova_cond_create(void) {
    nova_cond_t *cond = nova_alloc(sizeof(nova_cond_t));
    if (!cond) return NULL;

#if defined(NOVA_PLATFORM_POSIX)
    if (pthread_cond_init(&cond->cond, NULL) != 0) {
        nova_free(cond);
        return NULL;
    }
#elif defined(NOVA_PLATFORM_WINDOWS)
    InitializeConditionVariable(&cond->cond);
#endif

    return cond;
}

void nova_cond_destroy(nova_cond_t *cond) {
    if (!cond) return;

#if defined(NOVA_PLATFORM_POSIX)
    pthread_cond_destroy(&cond->cond);
#endif
    // Windows condition variables don't need explicit destruction

    nova_free(cond);
}

int nova_cond_wait(nova_cond_t *cond, nova_mutex_t *mutex) {
    if (!cond || !mutex) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return pthread_cond_wait(&cond->cond, &mutex->mutex) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return SleepConditionVariableCS(&cond->cond, &mutex->cs, INFINITE) ? 0 : -1;
#endif
}

int nova_cond_signal(nova_cond_t *cond) {
    if (!cond) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return pthread_cond_signal(&cond->cond) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    WakeConditionVariable(&cond->cond);
    return 0;
#endif
}

int nova_cond_broadcast(nova_cond_t *cond) {
    if (!cond) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return pthread_cond_broadcast(&cond->cond) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    WakeAllConditionVariable(&cond->cond);
    return 0;
#endif
}

int nova_cond_timed_wait(nova_cond_t *cond, nova_mutex_t *mutex, unsigned int timeout_ms) {
    if (!cond || !mutex) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    int result = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &ts);
    return (result == 0 || result == ETIMEDOUT) ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return SleepConditionVariableCS(&cond->cond, &mutex->cs, timeout_ms) ? 0 : -1;
#endif
}

/* ============================================================================
 * Thread Pool (Basic Implementation)
 * ========================================================================== */

static void thread_pool_worker_impl(void *arg) {
    nova_thread_pool_t *pool = (nova_thread_pool_t *)arg;

    while (true) {
        nova_mutex_lock(pool->mutex);

        while (pool->task_count == 0 && !pool->shutdown) {
            nova_cond_wait(pool->cond, pool->mutex);
        }

        if (pool->shutdown) {
            nova_mutex_unlock(pool->mutex);
            break;
        }

        // Get task
        nova_thread_task_t task = pool->tasks[pool->task_head];
        pool->task_head = (pool->task_head + 1) % pool->task_capacity;
        pool->task_count--;

        nova_mutex_unlock(pool->mutex);

        // Execute task
        task.func(task.arg);
    }
}

nova_thread_pool_t *nova_thread_pool_create(size_t num_threads) {
    if (num_threads == 0) return NULL;

    nova_thread_pool_t *pool = nova_alloc(sizeof(nova_thread_pool_t));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->threads = nova_alloc(sizeof(nova_thread_t *) * num_threads);
    pool->mutex = nova_mutex_create();
    pool->cond = nova_cond_create();
    pool->task_capacity = 1024; // Initial capacity
    pool->tasks = nova_alloc(sizeof(nova_thread_task_t) * pool->task_capacity);
    pool->task_count = 0;
    pool->task_head = 0;
    pool->task_tail = 0;
    pool->shutdown = false;

    if (!pool->threads || !pool->mutex || !pool->cond || !pool->tasks) {
        nova_thread_pool_destroy(pool);
        return NULL;
    }

    // Create worker threads
    for (size_t i = 0; i < num_threads; i++) {
        pool->threads[i] = nova_thread_create((nova_thread_func_t)thread_pool_worker_impl, pool);
        if (!pool->threads[i]) {
            nova_thread_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

void nova_thread_pool_destroy(nova_thread_pool_t *pool) {
    if (!pool) return;

    // Signal shutdown
    nova_mutex_lock(pool->mutex);
    pool->shutdown = true;
    nova_cond_broadcast(pool->cond);
    nova_mutex_unlock(pool->mutex);

    // Wait for threads to finish
    for (size_t i = 0; i < pool->num_threads; i++) {
        if (pool->threads[i]) {
            nova_thread_join(pool->threads[i]);
            nova_thread_destroy(pool->threads[i]);
        }
    }

    if (pool->threads) nova_free(pool->threads);
    if (pool->mutex) nova_mutex_destroy(pool->mutex);
    if (pool->cond) nova_cond_destroy(pool->cond);
    if (pool->tasks) nova_free(pool->tasks);

    nova_free(pool);
}

int nova_thread_pool_submit(nova_thread_pool_t *pool, nova_thread_func_t func, void *arg) {
    if (!pool || !func) return -1;

    nova_mutex_lock(pool->mutex);

    // Check if queue is full (simplified - would need to resize in real implementation)
    if (pool->task_count >= pool->task_capacity) {
        nova_mutex_unlock(pool->mutex);
        return -1;
    }

    // Add task
    pool->tasks[pool->task_tail].func = func;
    pool->tasks[pool->task_tail].arg = arg;
    pool->task_tail = (pool->task_tail + 1) % pool->task_capacity;
    pool->task_count++;

    nova_cond_signal(pool->cond);
    nova_mutex_unlock(pool->mutex);

    return 0;
}

int nova_thread_pool_wait(nova_thread_pool_t *pool) {
    if (!pool) return -1;

    // Simplified - in real implementation, would track active tasks
    nova_thread_sleep(10); // Small delay
    return 0;
}

/* ============================================================================
 * Atomic Operations
 * ========================================================================== */

void nova_atomic_init(nova_atomic_int_t *atomic, int value) {
    if (atomic) {
        atomic->value = value;
    }
}

int nova_atomic_load(nova_atomic_int_t *atomic) {
    if (!atomic) return 0;
    return atomic->value;
}

void nova_atomic_store(nova_atomic_int_t *atomic, int value) {
    if (atomic) {
        atomic->value = value;
    }
}

bool nova_atomic_compare_exchange(nova_atomic_int_t *atomic, int expected, int desired) {
    if (!atomic) return false;

#if defined(NOVA_PLATFORM_POSIX)
    return __sync_bool_compare_and_swap(&atomic->value, expected, desired);
#elif defined(NOVA_PLATFORM_WINDOWS)
    return InterlockedCompareExchange(&atomic->value, desired, expected) == expected;
#endif
}

int nova_atomic_increment(nova_atomic_int_t *atomic) {
    if (!atomic) return 0;

#if defined(NOVA_PLATFORM_POSIX)
    return __sync_add_and_fetch(&atomic->value, 1);
#elif defined(NOVA_PLATFORM_WINDOWS)
    return InterlockedIncrement(&atomic->value);
#endif
}

int nova_atomic_decrement(nova_atomic_int_t *atomic) {
    if (!atomic) return 0;

#if defined(NOVA_PLATFORM_POSIX)
    return __sync_sub_and_fetch(&atomic->value, 1);
#elif defined(NOVA_PLATFORM_WINDOWS)
    return InterlockedDecrement(&atomic->value);
#endif
}
