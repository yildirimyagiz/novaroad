/**
 * @file async.h
 * @brief Asynchronous runtime (event loop, coroutines, futures, promises)
 *
 * Nova async provides:
 * - Event loop for async I/O
 * - Stackful coroutines (fiber-like)
 * - Futures and promises (async/await)
 * - Channels for async communication
 * - Timers and schedulers
 * - Async combinators
 */

#ifndef NOVA_ASYNC_H
#define NOVA_ASYNC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nova_error nova_error_t;

/**
 * @brief Create error object
 * @param message Error message
 * @param code Error code
 * @return Error handle
 */
nova_error_t *nova_error_create(const char *message, nova_error_code_t code);

/**
 * @brief Destroy error object
 * @param error Error handle
 */
void nova_error_destroy(nova_error_t *error);

/**
 * @brief Get error code
 * @param error Error handle
 * @return Error code
 */
nova_error_code_t nova_error_code(nova_error_t *error);

/**
 * @brief Get error message
 * @param error Error handle
 * @return Error message
 */
const char *nova_error_message(nova_error_t *error);

/* ========================================================================
 * Event Loop
 * ======================================================================== */

typedef struct nova_event_loop nova_event_loop_t;
typedef void (*nova_event_callback_t)(void *userdata);

/* IO Event Flags */
#define NOVA_IO_READ 0x01
#define NOVA_IO_WRITE 0x02
#define NOVA_IO_ERROR 0x04

/* Callback Types */
typedef void (*nova_timer_callback_t)(nova_event_loop_t *loop, uint64_t timer_id, void *user_data);
typedef void (*nova_io_callback_t)(nova_event_loop_t *loop, int fd, int events, void *user_data);
typedef void (*nova_signal_callback_t)(nova_event_loop_t *loop, int signum, void *user_data);
typedef void (*nova_idle_callback_t)(nova_event_loop_t *loop, void *user_data);
typedef void (*nova_prepare_callback_t)(nova_event_loop_t *loop, void *user_data);
typedef void (*nova_check_callback_t)(nova_event_loop_t *loop, void *user_data);

/**
 * @brief Create event loop
 * @return Event loop handle, or NULL on failure
 */
nova_event_loop_t *nova_event_loop_create(void);

/**
 * @brief Schedule timer
 * @param loop Event loop handle
 * @param timeout_ms Delay in milliseconds
 * @param repeat_ms Repeat interval in milliseconds (0 for one-shot)
 * @param callback Callback function
 * @param user_data User data
 * @return Timer ID
 */
uint64_t nova_event_loop_timer(nova_event_loop_t *loop, uint64_t timeout_ms, uint64_t repeat_ms,
                               nova_timer_callback_t callback, void *user_data);

/**
 * @brief Stop timer
 * @param loop Event loop handle
 * @param timer_id Timer ID
 */
void nova_event_loop_timer_stop(nova_event_loop_t *loop, uint64_t timer_id);

/**
 * @brief Run event loop (blocks until stopped)
 * @param loop Event loop handle
 */
void nova_event_loop_run(nova_event_loop_t *loop);

/**
 * @brief Run event loop once (process pending events)
 * @param loop Event loop handle
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return Number of events processed
 */
int nova_event_loop_run_once(nova_event_loop_t *loop, int timeout_ms);

/**
 * @brief Stop event loop
 * @param loop Event loop handle
 */
void nova_event_loop_stop(nova_event_loop_t *loop);

/**
 * @brief Check if event loop is running
 * @param loop Event loop handle
 * @return true if running
 */
bool nova_event_loop_is_running(nova_event_loop_t *loop);

/**
 * @brief Schedule callback to run in event loop
 * @param loop Event loop handle
 * @param callback Callback function
 * @param userdata User data
 */
void nova_event_loop_schedule(nova_event_loop_t *loop, nova_event_callback_t callback,
                              void *userdata);

/**
 * @brief Get current event loop
 * @return Event loop handle, or NULL if not in event loop
 */
nova_event_loop_t *nova_event_loop_current(void);

/**
 * @brief Destroy event loop
 * @param loop Event loop handle
 */
void nova_event_loop_destroy(nova_event_loop_t *loop);

/* ========================================================================
 * Coroutines (Stackful)
 * ======================================================================== */

typedef struct nova_coroutine nova_coroutine_t;
typedef uint64_t nova_coroutine_id_t;
typedef void *(*nova_coroutine_func_t)(void *arg);

typedef enum {
    NOVA_COROUTINE_READY,
    NOVA_COROUTINE_RUNNING,
    NOVA_COROUTINE_SUSPENDED,
    NOVA_COROUTINE_FINISHED,
    NOVA_COROUTINE_CANCELLED
} nova_coroutine_state_t;

typedef struct {
    size_t total_futures;
    size_t active_futures;
    size_t completed_futures;
    size_t failed_futures;
} nova_future_stats_t;

typedef struct {
    nova_coroutine_id_t id;
    nova_coroutine_state_t state;
    uint64_t created_at;
    uint64_t started_at;
    uint64_t finished_at;
    uint64_t total_runtime;
    size_t yields;
    size_t resumes;
    size_t memory_allocated;
    size_t stack_size;
    bool is_cancelled;
} nova_coroutine_stats_t;

typedef struct {
    size_t num_coroutines;
    size_t active_coroutines;
    size_t total_created;
    size_t total_finished;
    size_t total_yields;
    size_t total_resumes;
} nova_scheduler_stats_t;

typedef void *(*nova_coro_func_t)(void *arg);

/**
 * @brief Create coroutine
 * @param func Coroutine entry point
 * @param arg Argument passed to function
 * @return Coroutine handle, or NULL on failure
 */
nova_coroutine_t *nova_coroutine_create(nova_coro_func_t func, void *arg);

/**
 * @brief Create coroutine with custom stack size
 * @param func Coroutine entry point
 * @param arg Argument
 * @param stack_size Stack size in bytes
 * @return Coroutine handle, or NULL on failure
 */
nova_coroutine_t *nova_coroutine_create_with_stack(nova_coro_func_t func, void *arg,
                                                   size_t stack_size);

/**
 * @brief Yield from current coroutine
 *
 * Transfers control back to the caller/scheduler.
 */
void nova_coroutine_yield(void);

/**
 * @brief Yield with value
 * @param value Value to return to caller
 */
void nova_coroutine_yield_value(void *value);

/**
 * @brief Resume coroutine
 * @param coro Coroutine handle
 * @return Value from yield, or NULL if finished
 */
void *nova_coroutine_resume(nova_coroutine_t *coro);

/**
 * @brief Check if coroutine is finished
 * @param coro Coroutine handle
 * @return true if finished
 */
bool nova_coroutine_is_finished(nova_coroutine_t *coro);

/**
 * @brief Get current coroutine
 * @return Current coroutine handle, or NULL if not in coroutine
 */
nova_coroutine_t *nova_coroutine_current(void);

/**
 * @brief Destroy coroutine
 * @param coro Coroutine handle
 */
void nova_coroutine_destroy(nova_coroutine_t *coro);

/* ========================================================================
 * Futures and Promises
 * ======================================================================== */

typedef struct nova_future nova_future_t;
typedef struct nova_promise nova_promise_t;

typedef enum {
    NOVA_FUTURE_PENDING,
    NOVA_FUTURE_FULFILLED,
    NOVA_FUTURE_REJECTED,
    NOVA_FUTURE_CANCELLED
} nova_future_state_t;

typedef void (*nova_future_callback_t)(nova_future_t *future, void *user_data);
typedef void (*nova_future_resolve_func_t)(void *result, void *user_data);
typedef void (*nova_future_reject_func_t)(void *error, void *user_data);

/**
 * @brief Create future
 * @return Future handle, or NULL on failure
 */
nova_future_t *nova_future_create(void);

/**
 * @brief Create future-promise pair
 * @param future Output: future handle
 * @param promise Output: promise handle
 * @return 0 on success, -1 on failure
 */
int nova_future_create_pair(nova_future_t **future, nova_promise_t **promise);

/**
 * @brief Check if future is ready
 * @param future Future handle
 * @return true if result is available
 */
bool nova_future_is_ready(nova_future_t *future);

/**
 * @brief Wait for future (blocks)
 * @param future Future handle
 * @return Result value
 */
void *nova_future_wait(nova_future_t *future);

/**
 * @brief Wait with timeout
 * @param future Future handle
 * @param timeout_ms Timeout in milliseconds
 * @param result Output: result value (if ready)
 * @return true if ready, false if timeout
 */
bool nova_future_wait_timeout(nova_future_t *future, uint32_t timeout_ms, void **result);

/**
 * @brief Set callback to run when future completes
 * @param future Future handle
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success
 */
int nova_future_then(nova_future_t *future, nova_future_callback_t callback, void *userdata);

/**
 * @brief Resolve promise with value
 * @param promise Promise handle
 * @param value Result value
 * @return 0 on success
 */
int nova_promise_resolve(nova_promise_t *promise, void *value);

/**
 * @brief Reject promise with error
 * @param promise Promise handle
 * @param error Error value
 * @return 0 on success
 */
int nova_promise_reject(nova_promise_t *promise, nova_error_t *error);

/**
 * @brief Destroy future
 * @param future Future handle
 */
void nova_future_destroy(nova_future_t *future);

/* ========================================================================
 * Async Combinators
 * ======================================================================== */

/**
 * @brief Wait for all futures to complete
 * @param futures Array of futures
 * @param count Number of futures
 * @return Combined future, or NULL on failure
 */
nova_future_t *nova_future_all(nova_future_t **futures, size_t count);

/**
 * @brief Wait for any future to complete
 * @param futures Array of futures
 * @param count Number of futures
 * @return Combined future (first to complete), or NULL on failure
 */
nova_future_t *nova_future_any(nova_future_t **futures, size_t count);

/**
 * @brief Race multiple futures
 * @param futures Array of futures
 * @param count Number of futures
 * @param index Output: index of winning future
 * @return Result from first future to complete
 */
void *nova_future_race(nova_future_t **futures, size_t count, size_t *index);

/* ========================================================================
 * Async Channels
 * ======================================================================== */

typedef struct nova_async_channel nova_async_channel_t;

/**
 * @brief Create async channel
 * @param capacity Channel capacity (0 for unbuffered)
 * @return Channel handle, or NULL on failure
 */
nova_async_channel_t *nova_async_channel_create(size_t capacity);

/**
 * @brief Send value to channel (async)
 * @param channel Channel handle
 * @param value Value to send
 * @return Future that resolves when sent
 */
nova_future_t *nova_async_channel_send(nova_async_channel_t *channel, void *value);

/**
 * @brief Receive from channel (async)
 * @param channel Channel handle
 * @return Future that resolves to received value
 */
nova_future_t *nova_async_channel_recv(nova_async_channel_t *channel);

/**
 * @brief Close channel
 * @param channel Channel handle
 */
void nova_async_channel_close(nova_async_channel_t *channel);

/**
 * @brief Destroy channel
 * @param channel Channel handle
 */
void nova_async_channel_destroy(nova_async_channel_t *channel);

/* ========================================================================
 * Timers
 * ======================================================================== */

typedef struct nova_timer nova_timer_t;

/**
 * @brief Create one-shot timer
 * @param loop Event loop handle
 * @param delay_ms Delay in milliseconds
 * @param callback Callback function
 * @param userdata User data
 * @return Timer handle, or NULL on failure
 */
nova_timer_t *nova_timer_once(nova_event_loop_t *loop, uint32_t delay_ms,
                              nova_event_callback_t callback, void *userdata);

/**
 * @brief Create repeating timer
 * @param loop Event loop handle
 * @param interval_ms Interval in milliseconds
 * @param callback Callback function
 * @param userdata User data
 * @return Timer handle, or NULL on failure
 */
nova_timer_t *nova_timer_repeat(nova_event_loop_t *loop, uint32_t interval_ms,
                                nova_event_callback_t callback, void *userdata);

/**
 * @brief Cancel timer
 * @param timer Timer handle
 */
void nova_timer_cancel(nova_timer_t *timer);

/**
 * @brief Destroy timer
 * @param timer Timer handle
 */
void nova_timer_destroy(nova_timer_t *timer);

/* ========================================================================
 * Sleep (Async)
 * ======================================================================== */

/**
 * @brief Async sleep
 * @param ms Milliseconds to sleep
 * @return Future that resolves after delay
 */
nova_future_t *nova_async_sleep(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_ASYNC_H */
