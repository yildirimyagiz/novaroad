/**
 * @file scheduler.c
 * @brief Process/thread scheduler implementation
 */

#include "kernel/sched.h"
#include "kernel/memory.h"
#include "scheduler.h"
#include <stddef.h>

#define MAX_TASKS 1024

struct nova_task {
    nova_tid_t tid;
    nova_task_state_t state;
    void *stack;
    void (*entry)(void *);
    void *arg;
    struct nova_task *next;
};

static struct {
    nova_task_t *tasks[MAX_TASKS];
    nova_task_t *current;
    nova_task_t *ready_queue;
    nova_tid_t next_tid;
} scheduler;

int nova_sched_init(void)
{
    scheduler.current = NULL;
    scheduler.ready_queue = NULL;
    scheduler.next_tid = 1;
    
    for (int i = 0; i < MAX_TASKS; i++) {
        scheduler.tasks[i] = NULL;
    }
    
    return 0;
}

nova_task_t *nova_task_create(void (*entry)(void *), void *arg)
{
    nova_task_t *task = nova_kmalloc(sizeof(nova_task_t));
    if (!task) return NULL;
    
    task->tid = scheduler.next_tid++;
    task->state = NOVA_TASK_RUNNABLE;
    task->entry = entry;
    task->arg = arg;
    task->stack = nova_kmalloc(8192); /* 8KB stack */
    task->next = NULL;
    
    /* Add to ready queue */
    if (scheduler.ready_queue == NULL) {
        scheduler.ready_queue = task;
    } else {
        nova_task_t *tail = scheduler.ready_queue;
        while (tail->next) tail = tail->next;
        tail->next = task;
    }
    
    return task;
}

void nova_sched_yield(void)
{
    /* TODO: Implement context switch */
}

void nova_sched_sleep(uint64_t ms)
{
    /* TODO: Implement sleep */
    (void)ms;
}

nova_task_t *nova_task_current(void)
{
    return scheduler.current;
}

nova_tid_t nova_task_get_tid(nova_task_t *task)
{
    return task ? task->tid : 0;
}
