/**
 * @file future.c
 * @brief Promise-based future implementation with continuations
 */

#include "runtime/async.h"
#include "runtime/thread.h"
#include "std/alloc.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct nova_error {
    char *message;
    nova_error_code_t code;
};

nova_error_t *nova_error_create(const char *message, nova_error_code_t code)
{
    nova_error_t *err = nova_alloc(sizeof(nova_error_t));
    if (err) {
        err->message = strdup(message);
        err->code = code;
    }
    return err;
}

void nova_error_destroy(nova_error_t *error)
{
    if (error) {
        free(error->message);
        nova_free(error);
    }
}

nova_error_code_t nova_error_code(nova_error_t *error)
{
    return error ? error->code : NOVA_ERROR_NONE;
}

const char *nova_error_message(nova_error_t *error)
{
    return error ? error->message : NULL;
}

// Forward declarations
void nova_future_retain(nova_future_t *future);
void nova_future_release(nova_future_t *future);
int nova_future_resolve_with(nova_future_t *future, void *result);
int nova_future_reject_with(nova_future_t *future, nova_error_t *error);
static void all_future_callback(nova_future_t *future, void *user_data);
static void race_future_callback(nova_future_t *future, void *user_data);
static void timeout_callback(nova_event_loop_t *loop, uint64_t timer_id, void *user_data);
static void async_task_wrapper(void *arg);

typedef void *(*nova_future_async_func_t)(void *arg);

// Future states
typedef enum { FUTURE_PENDING, FUTURE_FULFILLED, FUTURE_REJECTED, FUTURE_CANCELLED } future_state_t;

// Continuation callback
typedef struct future_continuation {
    nova_future_callback_t callback;
    void *user_data;
    struct future_continuation *next;
} future_continuation_t;

struct nova_future {
    atomic_int ref_count; // Reference counting
    future_state_t state;
    void *result;
    nova_error_t *error;

    // Continuations
    future_continuation_t *continuations;
    pthread_mutex_t continuation_mutex;

    // Parent promise (for chaining)
    struct nova_promise *promise;

    // Timing
    uint64_t created_at;
    uint64_t resolved_at;
};

struct nova_promise {
    nova_future_t future;
    nova_future_resolve_func_t resolve_func;
    nova_future_reject_func_t reject_func;
    void *user_data;
};

// Global future scheduler for async operations
static struct {
    // Thread pool for executing async tasks
    nova_thread_pool_t *thread_pool;

    // Statistics
    atomic_size_t total_futures;
    atomic_size_t active_futures;
    atomic_size_t completed_futures;
    atomic_size_t failed_futures;
} future_scheduler;

static uint64_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void nova_future_retain(nova_future_t *future)
{
    if (future) {
        atomic_fetch_add(&future->ref_count, 1);
    }
}

void nova_future_release(nova_future_t *future)
{
    if (!future)
        return;

    if (atomic_fetch_sub(&future->ref_count, 1) == 1) {
        // Free resources
        if (future->error) {
            nova_error_destroy(future->error);
        }

        // Free continuations
        future_continuation_t *cont = future->continuations;
        while (cont) {
            future_continuation_t *next = cont->next;
            nova_free(cont);
            cont = next;
        }

        nova_free(future);
    }
}

static void execute_continuations(nova_future_t *future)
{
    pthread_mutex_lock(&future->continuation_mutex);

    future_continuation_t *cont = future->continuations;
    future->continuations = NULL; // Clear list

    pthread_mutex_unlock(&future->continuation_mutex);

    // Execute continuations
    while (cont) {
        future_continuation_t *next = cont->next;

        // Call continuation callback
        cont->callback(future, cont->user_data);

        nova_free(cont);
        cont = next;
    }
}

nova_future_t *nova_future_create(void)
{
    nova_future_t *future = nova_alloc(sizeof(nova_future_t));
    if (!future)
        return NULL;

    memset(future, 0, sizeof(nova_future_t));

    atomic_init(&future->ref_count, 1);
    future->state = FUTURE_PENDING;
    future->created_at = get_time_ms();

    pthread_mutex_init(&future->continuation_mutex, NULL);

    atomic_fetch_add(&future_scheduler.total_futures, 1);
    atomic_fetch_add(&future_scheduler.active_futures, 1);

    return future;
}

void nova_future_destroy(nova_future_t *future)
{
    nova_future_release(future);
}

bool nova_future_is_ready(nova_future_t *future)
{
    return future && future->state != FUTURE_PENDING;
}

bool nova_future_is_fulfilled(nova_future_t *future)
{
    return future && future->state == FUTURE_FULFILLED;
}

bool nova_future_is_rejected(nova_future_t *future)
{
    return future && future->state == FUTURE_REJECTED;
}

bool nova_future_is_cancelled(nova_future_t *future)
{
    return future && future->state == FUTURE_CANCELLED;
}

nova_future_state_t nova_future_state(nova_future_t *future)
{
    if (!future)
        return NOVA_FUTURE_PENDING;

    switch (future->state) {
    case FUTURE_PENDING:
        return NOVA_FUTURE_PENDING;
    case FUTURE_FULFILLED:
        return NOVA_FUTURE_FULFILLED;
    case FUTURE_REJECTED:
        return NOVA_FUTURE_REJECTED;
    case FUTURE_CANCELLED:
        return NOVA_FUTURE_CANCELLED;
    default:
        return NOVA_FUTURE_PENDING;
    }
}

void *nova_future_result(nova_future_t *future)
{
    return future && future->state == FUTURE_FULFILLED ? future->result : NULL;
}

nova_error_t *nova_future_error(nova_future_t *future)
{
    return future && future->state == FUTURE_REJECTED ? future->error : NULL;
}

int nova_future_then(nova_future_t *future, nova_future_callback_t callback, void *user_data)
{
    if (!future || !callback)
        return -1;

    future_continuation_t *cont = nova_alloc(sizeof(future_continuation_t));
    if (!cont)
        return -1;

    cont->callback = callback;
    cont->user_data = user_data;
    cont->next = NULL;

    pthread_mutex_lock(&future->continuation_mutex);

    // Add to continuation list
    if (!future->continuations) {
        future->continuations = cont;
    } else {
        future_continuation_t *last = future->continuations;
        while (last->next)
            last = last->next;
        last->next = cont;
    }

    // If future is already resolved, execute continuation immediately
    bool should_execute = (future->state != FUTURE_PENDING);

    pthread_mutex_unlock(&future->continuation_mutex);

    if (should_execute) {
        cont->callback(future, user_data);
        nova_free(cont);
    }

    return 0;
}

int nova_future_catch(nova_future_t *future, nova_future_callback_t callback, void *user_data)
{
    // For simplicity, catch is same as then (in a real implementation, we'd distinguish)
    return nova_future_then(future, callback, user_data);
}

nova_future_t *nova_future_resolve(void *result)
{
    nova_future_t *future = nova_future_create();
    if (!future)
        return NULL;

    nova_future_resolve_with(future, result);
    return future;
}

nova_future_t *nova_future_reject(nova_error_t *error)
{
    nova_future_t *future = nova_future_create();
    if (!future)
        return NULL;

    nova_future_reject_with(future, error);
    return future;
}

int nova_future_resolve_with(nova_future_t *future, void *result)
{
    if (!future || future->state != FUTURE_PENDING)
        return -1;

    future->state = FUTURE_FULFILLED;
    future->result = result;
    future->resolved_at = get_time_ms();

    atomic_fetch_sub(&future_scheduler.active_futures, 1);
    atomic_fetch_add(&future_scheduler.completed_futures, 1);

    // Execute continuations
    execute_continuations(future);

    return 0;
}

int nova_future_reject_with(nova_future_t *future, nova_error_t *error)
{
    if (!future || future->state != FUTURE_PENDING)
        return -1;

    future->state = FUTURE_REJECTED;
    future->error = error;
    future->resolved_at = get_time_ms();

    atomic_fetch_sub(&future_scheduler.active_futures, 1);
    atomic_fetch_add(&future_scheduler.failed_futures, 1);

    // Execute continuations
    execute_continuations(future);

    return 0;
}

int nova_future_cancel(nova_future_t *future)
{
    if (!future || future->state != FUTURE_PENDING)
        return -1;

    future->state = FUTURE_CANCELLED;
    future->resolved_at = get_time_ms();

    atomic_fetch_sub(&future_scheduler.active_futures, 1);

    // Execute continuations
    execute_continuations(future);

    return 0;
}

void *nova_future_wait(nova_future_t *future)
{
    if (!future)
        return NULL;

    // Simple busy wait (in a real implementation, we'd use condition variables)
    while (future->state == FUTURE_PENDING) {
        usleep(1000); // 1ms
    }

    return nova_future_result(future);
}

nova_future_t *nova_future_all(nova_future_t **futures, size_t count)
{
    if (!futures || count == 0)
        return nova_future_resolve(NULL);

    nova_future_t *result_future = nova_future_create();
    if (!result_future)
        return NULL;

    // Track completion
    struct {
        nova_future_t **futures;
        size_t count;
        size_t completed;
        void **results;
        nova_future_t *result_future;
    } *ctx = nova_alloc(sizeof(*ctx));

    if (!ctx) {
        nova_future_destroy(result_future);
        return NULL;
    }

    ctx->futures = nova_alloc(count * sizeof(nova_future_t *));
    ctx->results = nova_alloc(count * sizeof(void *));
    if (!ctx->futures || !ctx->results) {
        nova_free(ctx->futures);
        nova_free(ctx->results);
        nova_free(ctx);
        nova_future_destroy(result_future);
        return NULL;
    }

    memcpy(ctx->futures, futures, count * sizeof(nova_future_t *));
    ctx->count = count;
    ctx->completed = 0;
    ctx->result_future = result_future;

    // Add continuations to all futures
    for (size_t i = 0; i < count; i++) {
        nova_future_retain(futures[i]);
        nova_future_then(futures[i], all_future_callback, ctx);
    }

    return result_future;
}

static void all_future_callback(nova_future_t *future, void *user_data)
{
    struct {
        nova_future_t **futures;
        size_t count;
        size_t completed;
        void **results;
        nova_future_t *result_future;
    } *ctx = user_data;

    size_t index = 0;
    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->futures[i] == future) {
            index = i;
            break;
        }
    }

    if (nova_future_is_fulfilled(future)) {
        ctx->results[index] = nova_future_result(future);
    } else {
        // If any future fails, reject the result
        nova_error_t *error = nova_future_error(future);
        nova_future_reject_with(ctx->result_future, error);
        goto cleanup;
    }

    ctx->completed++;
    if (ctx->completed == ctx->count) {
        // All futures completed successfully
        nova_future_resolve_with(ctx->result_future, ctx->results);
    }

cleanup:
    nova_future_release(future);
}

nova_future_t *nova_future_any(nova_future_t **futures, size_t count)
{
    if (!futures || count == 0)
        return nova_future_resolve(NULL);

    nova_future_t *result_future = nova_future_create();
    if (!result_future)
        return NULL;

    // Add continuations to all futures
    for (size_t i = 0; i < count; i++) {
        nova_future_retain(futures[i]);
        nova_future_then(futures[i], race_future_callback, result_future);
    }

    return result_future;
}

static void race_future_callback(nova_future_t *future, void *user_data)
{
    nova_future_t *result_future = user_data;

    // Only resolve/reject if result future is still pending
    if (nova_future_state(result_future) == NOVA_FUTURE_PENDING) {
        if (nova_future_is_fulfilled(future)) {
            nova_future_resolve_with(result_future, nova_future_result(future));
        } else {
            nova_error_t *error = nova_future_error(future);
            nova_future_reject_with(result_future, error);
        }
    }

    nova_future_release(future);
}

nova_future_t *nova_future_timeout(nova_future_t *future, uint64_t timeout_ms)
{
    nova_future_t *timeout_future = nova_future_create();
    if (!timeout_future)
        return NULL;

    // Schedule timeout
    nova_event_loop_timer(nova_event_loop_current(), timeout_ms, 0, timeout_callback,
                          timeout_future);

    // Race between original future and timeout
    nova_future_t *futures[2] = {future, timeout_future};
    nova_future_t *result = nova_future_any(futures, 2);

    nova_future_release(timeout_future); // Race retains it

    return result;
}

static void timeout_callback(nova_event_loop_t *loop, uint64_t timer_id, void *user_data)
{
    nova_future_t *future = user_data;

    if (nova_future_state(future) == NOVA_FUTURE_PENDING) {
        nova_error_t *error = nova_error_create("Operation timed out", NOVA_ERROR_TIMEOUT);
        nova_future_reject_with(future, error);
    }
}

nova_promise_t *nova_promise_create(void)
{
    nova_promise_t *promise = nova_alloc(sizeof(nova_promise_t));
    if (!promise)
        return NULL;

    // Initialize future part
    nova_future_t *future = &promise->future;
    memset(future, 0, sizeof(nova_future_t));

    atomic_init(&future->ref_count, 1);
    future->state = FUTURE_PENDING;
    future->created_at = get_time_ms();
    future->promise = promise;

    pthread_mutex_init(&future->continuation_mutex, NULL);

    atomic_fetch_add(&future_scheduler.total_futures, 1);
    atomic_fetch_add(&future_scheduler.active_futures, 1);

    return promise;
}

void nova_promise_destroy(nova_promise_t *promise)
{
    if (!promise)
        return;
    nova_future_release(&promise->future);
}

nova_future_t *nova_promise_future(nova_promise_t *promise)
{
    if (!promise)
        return NULL;
    nova_future_retain(&promise->future);
    return &promise->future;
}

int nova_promise_resolve(nova_promise_t *promise, void *result)
{
    if (!promise)
        return -1;
    return nova_future_resolve_with(&promise->future, result);
}

int nova_promise_reject(nova_promise_t *promise, nova_error_t *error)
{
    if (!promise)
        return -1;
    return nova_future_reject_with(&promise->future, error);
}

nova_future_stats_t nova_future_stats(void)
{
    nova_future_stats_t stats = {0};

    stats.total_futures = atomic_load(&future_scheduler.total_futures);
    stats.active_futures = atomic_load(&future_scheduler.active_futures);
    stats.completed_futures = atomic_load(&future_scheduler.completed_futures);
    stats.failed_futures = atomic_load(&future_scheduler.failed_futures);

    return stats;
}

void nova_future_system_init(void)
{
    memset(&future_scheduler, 0, sizeof(future_scheduler));

    // Create thread pool for async operations
    future_scheduler.thread_pool = nova_thread_pool_create(4);

    atomic_init(&future_scheduler.total_futures, 0);
    atomic_init(&future_scheduler.active_futures, 0);
    atomic_init(&future_scheduler.completed_futures, 0);
    atomic_init(&future_scheduler.failed_futures, 0);
}

void nova_future_system_shutdown(void)
{
    if (future_scheduler.thread_pool) {
        nova_thread_pool_destroy(future_scheduler.thread_pool);
        future_scheduler.thread_pool = NULL;
    }
}

nova_future_t *nova_future_async(nova_future_async_func_t func, void *arg)
{
    nova_promise_t *promise = nova_promise_create();
    if (!promise)
        return NULL;

    nova_future_t *future = nova_promise_future(promise);

    struct async_task_ctx {
        nova_future_async_func_t func;
        void *arg;
        nova_promise_t *promise;
    };

    struct async_task_ctx *ctx = nova_alloc(sizeof(struct async_task_ctx));
    if (!ctx) {
        nova_future_release(future);
        nova_promise_destroy(promise);
        return NULL;
    }

    ctx->func = func;
    ctx->arg = arg;
    ctx->promise = promise;

    // Submit async task to thread pool
    if (nova_thread_pool_submit(future_scheduler.thread_pool, async_task_wrapper, ctx) != 0) {
        nova_free(ctx);
        nova_future_release(future);
        nova_promise_destroy(promise);
        return NULL;
    }

    return future;
}

static void async_task_wrapper(void *arg)
{
    struct async_task_ctx {
        nova_future_async_func_t func;
        void *arg;
        nova_promise_t *promise;
    } *ctx = arg;

    void *result = ctx->func(ctx->arg);

    if (result) {
        nova_promise_resolve(ctx->promise, result);
    } else {
        nova_error_t *error = nova_error_create("Async operation failed", NOVA_ERROR_UNKNOWN);
        nova_promise_reject(ctx->promise, error);
    }

    nova_promise_destroy(ctx->promise);
    nova_free(ctx);
}
