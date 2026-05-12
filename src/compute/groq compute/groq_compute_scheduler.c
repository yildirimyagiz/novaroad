#include <stdio.h>
#include <stdlib.h>

// Grok Compute Scheduler - Work Stealing, Backend Selection
// Integrates WSS and AFC from Nova Engine

typedef struct Task {
    void (*func)(void);
    void *data;
} Task;

typedef struct {
    Task *tasks;
    int head, tail, capacity;
} TaskQueue;

TaskQueue *groq_create_queue(int cap) {
    TaskQueue *q = malloc(sizeof(TaskQueue));
    q->tasks = malloc(sizeof(Task) * cap);
    q->head = q->tail = 0;
    q->capacity = cap;
    return q;
}

void groq_enqueue(TaskQueue *q, Task t) {
    q->tasks[q->tail++] = t;
    if (q->tail >= q->capacity) q->tail = 0;
}

Task groq_dequeue(TaskQueue *q) {
    if (q->head == q->tail) return (Task){NULL, NULL};
    Task t = q->tasks[q->head++];
    if (q->head >= q->capacity) q->head = 0;
    return t;
}

void groq_steal(TaskQueue *from, TaskQueue *to) {
    Task t = groq_dequeue(from);
    if (t.func) groq_enqueue(to, t);
    printf("Grok Compute: Task stolen\n");
}
