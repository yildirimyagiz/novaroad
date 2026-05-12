/**
 * @file sched.h
 * @brief Process and thread scheduler
 */

#ifndef NOVA_SCHED_H
#define NOVA_SCHED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t nova_pid_t;
typedef uint64_t nova_tid_t;

typedef enum {
    NOVA_TASK_RUNNABLE,
    NOVA_TASK_RUNNING,
    NOVA_TASK_BLOCKED,
    NOVA_TASK_SLEEPING,
    NOVA_TASK_ZOMBIE,
    NOVA_TASK_DEAD,
} nova_task_state_t;

typedef enum {
    NOVA_SCHED_FAIR,        /**< Fair scheduling (CFS-like) */
    NOVA_SCHED_REALTIME,    /**< Real-time scheduling */
    NOVA_SCHED_BATCH,       /**< Batch/background tasks */
} nova_sched_policy_t;

typedef struct nova_task nova_task_t;

/* ============================================================================
 * Scheduler Initialization
 * ========================================================================== */

/**
 * Initialize scheduler
 * @return 0 on success
 */
int nova_sched_init(void);

/**
 * Start scheduler (begins multitasking)
 */
void nova_sched_start(void);

/* ============================================================================
 * Task Creation
 * ========================================================================== */

/**
 * Create new task
 * @param entry Entry point function
 * @param arg Argument to entry point
 * @return Task handle or NULL on error
 */
nova_task_t *nova_task_create(void (*entry)(void *), void *arg);

/**
 * Create task with priority
 * @param entry Entry point
 * @param arg Argument
 * @param priority Priority (0-99, higher = more priority)
 * @return Task handle or NULL
 */
nova_task_t *nova_task_create_priority(void (*entry)(void *), void *arg, int priority);

/**
 * Fork current task
 * @return Child PID in parent, 0 in child, -1 on error
 */
nova_pid_t nova_task_fork(void);

/* ============================================================================
 * Task Management
 * ========================================================================== */

/**
 * Get current task
 * @return Current task handle
 */
nova_task_t *nova_task_current(void);

/**
 * Get task by PID
 * @param pid Process ID
 * @return Task handle or NULL
 */
nova_task_t *nova_task_by_pid(nova_pid_t pid);

/**
 * Get task ID
 * @param task Task handle
 * @return Task ID
 */
nova_tid_t nova_task_get_tid(nova_task_t *task);

/**
 * Get task state
 * @param task Task handle
 * @return Task state
 */
nova_task_state_t nova_task_get_state(nova_task_t *task);

/**
 * Set task priority
 * @param task Task handle
 * @param priority New priority (0-99)
 */
void nova_task_set_priority(nova_task_t *task, int priority);

/**
 * Get task priority
 * @param task Task handle
 * @return Current priority
 */
int nova_task_get_priority(nova_task_t *task);

/* ============================================================================
 * Scheduling Operations
 * ========================================================================== */

/**
 * Yield CPU to scheduler
 */
void nova_sched_yield(void);

/**
 * Sleep for milliseconds
 * @param ms Milliseconds to sleep
 */
void nova_sched_sleep(uint64_t ms);

/**
 * Block current task (wait for wakeup)
 */
void nova_task_block(void);

/**
 * Wake up task
 * @param task Task to wake
 */
void nova_task_wake(nova_task_t *task);

/**
 * Exit current task
 * @param exit_code Exit code
 */
void nova_task_exit(int exit_code) __attribute__((noreturn));

/**
 * Kill task
 * @param task Task to kill
 * @return 0 on success, -1 on error
 */
int nova_task_kill(nova_task_t *task);

/**
 * Wait for task to finish
 * @param task Task to wait for
 * @param exit_code Output: exit code
 * @return 0 on success, -1 on error
 */
int nova_task_wait(nova_task_t *task, int *exit_code);

/* ============================================================================
 * CPU Affinity
 * ========================================================================== */

/**
 * Set task CPU affinity
 * @param task Task handle
 * @param cpu CPU ID
 * @return 0 on success, -1 on error
 */
int nova_task_set_affinity(nova_task_t *task, uint32_t cpu);

/**
 * Get task CPU affinity
 * @param task Task handle
 * @return CPU ID or -1
 */
int nova_task_get_affinity(nova_task_t *task);

/* ============================================================================
 * Scheduler Statistics
 * ========================================================================== */

typedef struct {
    uint64_t runtime_ns;        /**< Total runtime in nanoseconds */
    uint64_t switches;          /**< Number of context switches */
    uint64_t yields;            /**< Number of voluntary yields */
    uint64_t preemptions;       /**< Number of preemptions */
} nova_task_stats_t;

/**
 * Get task statistics
 * @param task Task handle
 * @param stats Output statistics
 */
void nova_task_get_stats(nova_task_t *task, nova_task_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_SCHED_H */
