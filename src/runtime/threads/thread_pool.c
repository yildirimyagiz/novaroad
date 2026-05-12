/**
 * @file thread_pool.c
 * @brief High-performance thread pool with work-stealing
 */

#include "runtime/thread.h"
#include "std/alloc.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_THREADS 256
#define TASK_QUEUE_SIZE 1024
#define WORK_STEALING_ATTEMPTS 3

typedef struct nova_task {
    nova_task_fn_t function;
    void *arg;
    struct nova_task *next;
} nova_task_t;

typedef struct {
    nova_task_t *head;
    nova_task_t *tail;
    atomic_size_t size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} task_queue_t;

typedef struct {
    pthread_t thread;
    int thread_id;
    task_queue_t *local_queue;
    atomic_bool is_active;
    struct nova_thread_pool *pool;
} worker_thread_t;

struct nova_thread_pool {
    worker_thread_t *workers;
    task_queue_t *queues;
    size_t num_threads;
    atomic_bool running;
    atomic_size_t active_tasks;
    pthread_mutex_t pool_mutex;
    pthread_cond_t pool_cond;

    // Statistics
    atomic_size_t total_tasks;
    atomic_size_t stolen_tasks;
    atomic_size_t completed_tasks;
};

static void task_queue_init(task_queue_t *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    atomic_init(&queue->size, 0);
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

static void task_queue_destroy(task_queue_t *queue)
{
    // Free remaining tasks
    nova_task_t *task = queue->head;
    while (task) {
        nova_task_t *next = task->next;
        nova_free(task);
        task = next;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

static bool task_queue_push(task_queue_t *queue, nova_task_fn_t function, void *arg)
{
    nova_task_t *task = nova_alloc(sizeof(nova_task_t));
    if (!task)
        return false;

    task->function = function;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&queue->mutex);

    // Wait if queue is full (simplified - no actual limit check)
    while (atomic_load(&queue->size) >= TASK_QUEUE_SIZE) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->tail) {
        queue->tail->next = task;
    } else {
        queue->head = task;
    }
    queue->tail = task;

    atomic_fetch_add(&queue->size, 1);
    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);
    return true;
}

static nova_task_t *task_queue_pop(task_queue_t *queue)
{
    pthread_mutex_lock(&queue->mutex);

    while (atomic_load(&queue->size) == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    nova_task_t *task = queue->head;
    if (task) {
        queue->head = task->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
        atomic_fetch_sub(&queue->size, 1);
        pthread_cond_signal(&queue->not_full);
    }

    pthread_mutex_unlock(&queue->mutex);
    return task;
}

static nova_task_t *task_queue_steal(task_queue_t *queue)
{
    pthread_mutex_lock(&queue->mutex);

    nova_task_t *task = NULL;
    if (atomic_load(&queue->size) > 0) {
        // Steal from the opposite end (LIFO for thief, FIFO for owner)
        task = queue->tail;
        if (task) {
            if (task == queue->head) {
                queue->head = queue->tail = NULL;
            } else {
                // Find the task before tail
                nova_task_t *prev = queue->head;
                while (prev && prev->next != task) {
                    prev = prev->next;
                }
                if (prev) {
                    prev->next = NULL;
                    queue->tail = prev;
                }
            }
            atomic_fetch_sub(&queue->size, 1);
            pthread_cond_signal(&queue->not_full);
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return task;
}

static void *worker_thread_func(void *arg)
{
    worker_thread_t *worker = (worker_thread_t *) arg;
    nova_thread_pool_t *pool = worker->pool;

    while (atomic_load(&pool->running)) {
        nova_task_t *task = NULL;

        // First, try to get task from local queue
        task = task_queue_pop(worker->local_queue);

        // If no local task, try work-stealing
        if (!task) {
            for (int attempt = 0; attempt < WORK_STEALING_ATTEMPTS && !task; attempt++) {
                int victim_id = rand() % (int) pool->num_threads;
                if (victim_id != worker->thread_id) {
                    task = task_queue_steal(&pool->queues[victim_id]);
                    if (task) {
                        atomic_fetch_add(&pool->stolen_tasks, 1);
                    }
                }
            }
        }

        if (task) {
            // Execute task
            atomic_fetch_add(&pool->active_tasks, 1);

            task->function(task->arg);

            atomic_fetch_add(&pool->active_tasks, -1);
            atomic_fetch_add(&pool->completed_tasks, 1);

            nova_free(task);
        } else {
            // No work available, sleep briefly
            usleep(1000); // 1ms
        }
    }

    return NULL;
}

nova_thread_pool_t *nova_thread_pool_create(size_t num_threads)
{
    if (num_threads <= 0) {
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_threads <= 0)
            num_threads = 4;
    }

    if (num_threads > MAX_THREADS) {
        num_threads = MAX_THREADS;
    }

    nova_thread_pool_t *pool = nova_alloc(sizeof(nova_thread_pool_t));
    if (!pool)
        return NULL;

    memset(pool, 0, sizeof(nova_thread_pool_t));

    pool->num_threads = num_threads;
    atomic_init(&pool->running, true);
    atomic_init(&pool->active_tasks, 0);
    atomic_init(&pool->total_tasks, 0);
    atomic_init(&pool->stolen_tasks, 0);
    atomic_init(&pool->completed_tasks, 0);

    pthread_mutex_init(&pool->pool_mutex, NULL);
    pthread_cond_init(&pool->pool_cond, NULL);

    // Allocate workers and queues
    pool->workers = nova_alloc(sizeof(worker_thread_t) * num_threads);
    pool->queues = nova_alloc(sizeof(task_queue_t) * num_threads);

    if (!pool->workers || !pool->queues) {
        nova_free(pool->workers);
        nova_free(pool->queues);
        nova_free(pool);
        return NULL;
    }

    // Initialize queues
    for (int i = 0; i < num_threads; i++) {
        task_queue_init(&pool->queues[i]);
    }

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        pool->workers[i].thread_id = i;
        pool->workers[i].local_queue = &pool->queues[i];
        pool->workers[i].pool = pool;
        atomic_init(&pool->workers[i].is_active, true);

        if (pthread_create(&pool->workers[i].thread, NULL, worker_thread_func, &pool->workers[i]) !=
            0) {
            // Cleanup on failure
            atomic_store(&pool->running, false);
            nova_thread_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

void nova_thread_pool_destroy(nova_thread_pool_t *pool)
{
    if (!pool)
        return;

    // Signal threads to stop
    atomic_store(&pool->running, false);

    // Wake up all threads
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_cond_broadcast(&pool->queues[i].not_empty);
    }

    // Wait for threads to finish
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->workers[i].thread, NULL);
    }

    // Clean up queues
    for (int i = 0; i < pool->num_threads; i++) {
        task_queue_destroy(&pool->queues[i]);
    }

    // Clean up resources
    nova_free(pool->workers);
    nova_free(pool->queues);
    pthread_mutex_destroy(&pool->pool_mutex);
    pthread_cond_destroy(&pool->pool_cond);
    nova_free(pool);
}

int nova_thread_pool_submit(nova_thread_pool_t *pool, nova_task_fn_t function, void *arg)
{
    if (!pool || !function)
        return false;

    // Choose a queue using simple round-robin (could be improved)
    static atomic_int next_queue = 0;
    int queue_id = atomic_fetch_add(&next_queue, 1) % pool->num_threads;

    if (task_queue_push(&pool->queues[queue_id], function, arg)) {
        atomic_fetch_add(&pool->total_tasks, 1);
        return 0;
    }

    return -1;
}

void nova_thread_pool_wait(nova_thread_pool_t *pool)
{
    if (!pool)
        return;

    pthread_mutex_lock(&pool->pool_mutex);

    while (atomic_load(&pool->active_tasks) > 0 ||
           atomic_load(&pool->total_tasks) != atomic_load(&pool->completed_tasks)) {
        pthread_cond_wait(&pool->pool_cond, &pool->pool_mutex);
    }

    pthread_mutex_unlock(&pool->pool_mutex);
}

size_t nova_thread_pool_active_tasks(nova_thread_pool_t *pool)
{
    return pool ? atomic_load(&pool->active_tasks) : 0;
}

size_t nova_thread_pool_pending_tasks(nova_thread_pool_t *pool)
{
    if (!pool)
        return 0;

    size_t pending = 0;
    for (int i = 0; i < pool->num_threads; i++) {
        pending += atomic_load(&pool->queues[i].size);
    }
    return pending;
}

void nova_thread_pool_stats(nova_thread_pool_t *pool, nova_thread_pool_stats_t *stats)
{
    if (!pool || !stats)
        return;

    stats->num_threads = pool->num_threads;
    stats->active_tasks = atomic_load(&pool->active_tasks);
    stats->pending_tasks = nova_thread_pool_pending_tasks(pool);
    stats->total_tasks = atomic_load(&pool->total_tasks);
    stats->completed_tasks = atomic_load(&pool->completed_tasks);
    stats->stolen_tasks = atomic_load(&pool->stolen_tasks);
}

/* Legacy compatibility */
void nova_thread_pool_init(void)
{
    /* No-op - thread pools should be created explicitly */
}
