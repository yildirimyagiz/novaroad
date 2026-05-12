/**
 * @file scheduler.h
 * @brief Scheduler internal definitions
 */

#ifndef NOVA_KERNEL_SCHEDULER_H
#define NOVA_KERNEL_SCHEDULER_H

#include "kernel/sched.h"

/* Internal scheduler functions */
void nova_sched_switch(nova_task_t *from, nova_task_t *to);

#endif /* NOVA_KERNEL_SCHEDULER_H */
