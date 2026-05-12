/**
 * @file coroutine.c
 * @brief Stackful coroutine implementation with assembly context switching
 */

#include "runtime/async.h"
#include "std/alloc.h"
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#ifndef MAP_STACK
#define MAP_STACK 0
#endif

#include <unistd.h>

#define COROUTINE_STACK_SIZE (64 * 1024) // 64KB stack
#define MAX_COROUTINES 10000

// CPU register context for context switching
typedef struct {
    void *rsp; // Stack pointer
    void *rbp; // Base pointer
    void *rbx; // General purpose
    void *r12; // General purpose
    void *r13; // General purpose
    void *r14; // General purpose
    void *r15; // General purpose
    void *rip; // Instruction pointer
} cpu_context_t;

// Coroutine states
typedef enum {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED,
    COROUTINE_FINISHED,
    COROUTINE_CANCELLED
} coroutine_state_t;

struct nova_coroutine {
    nova_coroutine_id_t id;
    nova_coroutine_func_t function;
    void *arg;
    void *result;

    // Stack management
    void *stack_base;
    size_t stack_size;
    cpu_context_t context;

    // State management
    coroutine_state_t state;
    atomic_bool cancelled;
    int exit_code;

    // Scheduling
    struct nova_coroutine *next;
    struct nova_coroutine *prev;

    // Parent/child relationships
    struct nova_coroutine *parent;
    struct nova_coroutine *first_child;
    struct nova_coroutine *next_sibling;

    // Timing
    uint64_t created_at;
    uint64_t started_at;
    uint64_t finished_at;
    uint64_t total_runtime;

    // Statistics
    size_t yields;
    size_t resumes;
    size_t memory_allocated;
};

// Global coroutine scheduler
static struct {
    struct nova_coroutine *current;
    struct nova_coroutine *main_coroutine;
    struct nova_coroutine *ready_queue;
    struct nova_coroutine *finished_queue;

    atomic_size_t num_coroutines;
    atomic_size_t active_coroutines;
    pthread_mutex_t scheduler_mutex;
    pthread_cond_t scheduler_cond;

    // Statistics
    atomic_size_t total_yields;
    atomic_size_t total_resumes;
    atomic_size_t total_created;
    atomic_size_t total_finished;
} scheduler;

// Assembly context switching functions (platform-specific)
extern void nova_coroutine_context_switch(cpu_context_t *from, cpu_context_t *to);
extern void nova_coroutine_context_init(cpu_context_t *context, void *stack_top,
                                        void (*func)(void *), void *arg);

static nova_coroutine_id_t generate_coroutine_id(void)
{
    static atomic_size_t next_id = 1;
    return atomic_fetch_add(&next_id, 1);
}

static void coroutine_wrapper(void *arg)
{
    struct nova_coroutine *coro = (struct nova_coroutine *) arg;

    coro->started_at = nova_get_time_ns();

    // Call the coroutine function
    coro->result = coro->function(coro->arg);

    coro->finished_at = nova_get_time_ns();
    coro->total_runtime = coro->finished_at - coro->started_at;
    coro->state = COROUTINE_FINISHED;

    // Yield back to scheduler
    nova_coroutine_yield();
}

static struct nova_coroutine *create_coroutine(nova_coroutine_func_t func, void *arg,
                                               size_t stack_size)
{
    if (!func)
        return NULL;

    struct nova_coroutine *coro = nova_alloc(sizeof(struct nova_coroutine));
    if (!coro)
        return NULL;

    memset(coro, 0, sizeof(struct nova_coroutine));

    coro->id = generate_coroutine_id();
    coro->function = func;
    coro->arg = arg;
    coro->state = COROUTINE_READY;
    coro->created_at = nova_get_time_ns();

    // Allocate stack
    coro->stack_size = stack_size ? stack_size : COROUTINE_STACK_SIZE;

    // Use mmap for stack allocation to ensure proper alignment
    coro->stack_base = mmap(NULL, coro->stack_size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (coro->stack_base == MAP_FAILED) {
        nova_free(coro);
        return NULL;
    }

    // Initialize CPU context
    void *stack_top = coro->stack_base + coro->stack_size;
    nova_coroutine_context_init(&coro->context, stack_top, coroutine_wrapper, coro);

    atomic_init(&coro->cancelled, false);

    return coro;
}

static void destroy_coroutine(struct nova_coroutine *coro)
{
    if (!coro)
        return;

    // Free stack
    if (coro->stack_base) {
        munmap(coro->stack_base, coro->stack_size);
    }

    nova_free(coro);
}

static void scheduler_enqueue(struct nova_coroutine *coro)
{
    if (!coro)
        return;

    coro->next = scheduler.ready_queue;
    if (scheduler.ready_queue) {
        scheduler.ready_queue->prev = coro;
    }
    scheduler.ready_queue = coro;
    coro->prev = NULL;
}

static struct nova_coroutine *scheduler_dequeue(void)
{
    if (!scheduler.ready_queue)
        return NULL;

    struct nova_coroutine *coro = scheduler.ready_queue;
    scheduler.ready_queue = coro->next;
    if (scheduler.ready_queue) {
        scheduler.ready_queue->prev = NULL;
    }
    coro->next = coro->prev = NULL;

    return coro;
}

static void scheduler_add_finished(struct nova_coroutine *coro)
{
    if (!coro)
        return;

    coro->next = scheduler.finished_queue;
    scheduler.finished_queue = coro;
}

nova_coroutine_t *nova_coroutine_create(nova_coroutine_func_t func, void *arg)
{
    return nova_coroutine_create_with_stack(func, arg, 0);
}

nova_coroutine_t *nova_coroutine_create_with_stack(nova_coroutine_func_t func, void *arg,
                                                   size_t stack_size)
{
    pthread_mutex_lock(&scheduler.scheduler_mutex);

    struct nova_coroutine *coro = create_coroutine(func, arg, stack_size);
    if (!coro) {
        pthread_mutex_unlock(&scheduler.scheduler_mutex);
        return NULL;
    }

    // Add to ready queue
    scheduler_enqueue(coro);
    atomic_fetch_add(&scheduler.num_coroutines, 1);
    atomic_fetch_add(&scheduler.total_created, 1);

    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    return (nova_coroutine_t *) coro;
}

void nova_coroutine_destroy(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    if (!c)
        return;

    pthread_mutex_lock(&scheduler.scheduler_mutex);

    // Remove from queues if present
    if (c->next || c->prev || scheduler.ready_queue == c) {
        if (c->prev) {
            c->prev->next = c->next;
        } else {
            scheduler.ready_queue = c->next;
        }
        if (c->next) {
            c->next->prev = c->prev;
        }
    }

    atomic_fetch_sub(&scheduler.num_coroutines, 1);

    if (c->state == COROUTINE_FINISHED) {
        atomic_fetch_add(&scheduler.total_finished, 1);
    }

    destroy_coroutine(c);

    pthread_mutex_unlock(&scheduler.scheduler_mutex);
}

void *nova_coroutine_resume(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    if (!c || c->state != COROUTINE_READY)
        return NULL;

    pthread_mutex_lock(&scheduler.scheduler_mutex);

    struct nova_coroutine *prev = scheduler.current;
    scheduler.current = c;
    c->state = COROUTINE_RUNNING;
    c->resumes++;

    atomic_fetch_add(&scheduler.total_resumes, 1);

    // Context switch to coroutine
    nova_coroutine_context_switch(&prev->context, &c->context);

    // Back from coroutine
    if (c->state == COROUTINE_FINISHED) {
        scheduler_add_finished(c);
    } else if (c->state == COROUTINE_SUSPENDED) {
        scheduler_enqueue(c);
    }

    scheduler.current = prev;

    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    return c->result;
}

void nova_coroutine_yield(void)
{
    struct nova_coroutine *c = scheduler.current;
    if (!c || c->state != COROUTINE_RUNNING)
        return;

    c->state = COROUTINE_SUSPENDED;
    c->yields++;
    atomic_fetch_add(&scheduler.total_yields, 1);

    // Find next coroutine to run
    struct nova_coroutine *next = scheduler_dequeue();
    if (!next) {
        // No more coroutines, yield to main
        next = scheduler.main_coroutine;
    }

    struct nova_coroutine *prev = c;
    scheduler.current = next;
    next->state = COROUTINE_RUNNING;

    // Context switch
    nova_coroutine_context_switch(&prev->context, &next->context);
}

nova_coroutine_id_t nova_coroutine_id(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    return c ? c->id : 0;
}

nova_coroutine_state_t nova_coroutine_state(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    if (!c)
        return NOVA_COROUTINE_FINISHED;

    switch (c->state) {
    case COROUTINE_READY:
        return NOVA_COROUTINE_READY;
    case COROUTINE_RUNNING:
        return NOVA_COROUTINE_RUNNING;
    case COROUTINE_SUSPENDED:
        return NOVA_COROUTINE_SUSPENDED;
    case COROUTINE_FINISHED:
        return NOVA_COROUTINE_FINISHED;
    case COROUTINE_CANCELLED:
        return NOVA_COROUTINE_CANCELLED;
    default:
        return NOVA_COROUTINE_FINISHED;
    }
}

void *nova_coroutine_result(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    return c && c->state == COROUTINE_FINISHED ? c->result : NULL;
}

void nova_coroutine_cancel(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    if (!c)
        return;

    atomic_store(&c->cancelled, true);
    c->state = COROUTINE_CANCELLED;
}

bool nova_coroutine_is_cancelled(nova_coroutine_t *coro)
{
    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    return c ? atomic_load(&c->cancelled) : false;
}

nova_coroutine_stats_t nova_coroutine_stats(nova_coroutine_t *coro)
{
    nova_coroutine_stats_t stats = {0};

    struct nova_coroutine *c = (struct nova_coroutine *) coro;
    if (!c)
        return stats;

    stats.id = c->id;
    stats.state = nova_coroutine_state(coro);
    stats.created_at = c->created_at;
    stats.started_at = c->started_at;
    stats.finished_at = c->finished_at;
    stats.total_runtime = c->total_runtime;
    stats.yields = c->yields;
    stats.resumes = c->resumes;
    stats.memory_allocated = c->memory_allocated;
    stats.stack_size = c->stack_size;
    stats.is_cancelled = atomic_load(&c->cancelled);

    return stats;
}

void nova_coroutine_scheduler_run(void)
{
    while (1) {
        pthread_mutex_lock(&scheduler.scheduler_mutex);

        struct nova_coroutine *coro = scheduler_dequeue();
        if (!coro) {
            // No more coroutines to run
            pthread_mutex_unlock(&scheduler.scheduler_mutex);
            break;
        }

        struct nova_coroutine *prev = scheduler.current;
        scheduler.current = coro;
        coro->state = COROUTINE_RUNNING;

        pthread_mutex_unlock(&scheduler.scheduler_mutex);

        // Resume coroutine
        nova_coroutine_context_switch(&prev->context, &coro->context);

        pthread_mutex_lock(&scheduler.scheduler_mutex);

        // Handle coroutine completion
        if (coro->state == COROUTINE_FINISHED) {
            scheduler_add_finished(coro);
            atomic_fetch_add(&scheduler.total_finished, 1);
        } else if (coro->state == COROUTINE_SUSPENDED) {
            scheduler_enqueue(coro);
        }

        scheduler.current = prev;

        pthread_mutex_unlock(&scheduler.scheduler_mutex);
    }
}

size_t nova_coroutine_count(void)
{
    return atomic_load(&scheduler.num_coroutines);
}

nova_coroutine_id_t *nova_coroutine_list(size_t *count)
{
    if (!count)
        return NULL;

    pthread_mutex_lock(&scheduler.scheduler_mutex);

    size_t num = atomic_load(&scheduler.num_coroutines);
    *count = num;

    if (num == 0) {
        pthread_mutex_unlock(&scheduler.scheduler_mutex);
        return NULL;
    }

    nova_coroutine_id_t *ids = nova_alloc(num * sizeof(nova_coroutine_id_t));
    if (!ids) {
        pthread_mutex_unlock(&scheduler.scheduler_mutex);
        return NULL;
    }

    // Walk ready queue
    size_t i = 0;
    struct nova_coroutine *coro = scheduler.ready_queue;
    while (coro && i < num) {
        ids[i++] = coro->id;
        coro = coro->next;
    }

    // Add current if not in queue
    if (scheduler.current && i < num) {
        ids[i++] = scheduler.current->id;
    }

    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    return ids;
}

nova_scheduler_stats_t nova_scheduler_stats(void)
{
    nova_scheduler_stats_t stats = {0};

    stats.num_coroutines = atomic_load(&scheduler.num_coroutines);
    stats.active_coroutines = atomic_load(&scheduler.active_coroutines);
    stats.total_created = atomic_load(&scheduler.total_created);
    stats.total_finished = atomic_load(&scheduler.total_finished);
    stats.total_yields = atomic_load(&scheduler.total_yields);
    stats.total_resumes = atomic_load(&scheduler.total_resumes);

    return stats;
}

void nova_coroutine_system_init(void)
{
    memset(&scheduler, 0, sizeof(scheduler));

    // Create main coroutine context
    scheduler.main_coroutine = nova_alloc(sizeof(struct nova_coroutine));
    memset(scheduler.main_coroutine, 0, sizeof(struct nova_coroutine));
    scheduler.main_coroutine->state = COROUTINE_RUNNING;
    scheduler.current = scheduler.main_coroutine;

    pthread_mutex_init(&scheduler.scheduler_mutex, NULL);
    pthread_cond_init(&scheduler.scheduler_cond, NULL);
}

void nova_coroutine_system_shutdown(void)
{
    // Clean up all remaining coroutines
    pthread_mutex_lock(&scheduler.scheduler_mutex);

    struct nova_coroutine *coro = scheduler.ready_queue;
    while (coro) {
        struct nova_coroutine *next = coro->next;
        destroy_coroutine(coro);
        coro = next;
    }

    coro = scheduler.finished_queue;
    while (coro) {
        struct nova_coroutine *next = coro->next;
        destroy_coroutine(coro);
        coro = next;
    }

    if (scheduler.main_coroutine) {
        nova_free(scheduler.main_coroutine);
    }

    pthread_mutex_unlock(&scheduler.scheduler_mutex);
    pthread_mutex_destroy(&scheduler.scheduler_mutex);
    pthread_cond_destroy(&scheduler.scheduler_cond);
}
