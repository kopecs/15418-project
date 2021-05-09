/**
 * @file task_index.c
 */

#include <time.h>
#include <stdlib.h>

#include "tasks.h"

/** Index of task costs */
struct task_cost *task_index;

void task_cost_measure(struct task *t) {
    clock_t start, end;

    // Allocate index node
    struct task_cost *cost = malloc(sizeof(struct task_cost));
    if (!cost) {
        return;
    }

    // Initialize index node
    cost->fn = t->fn;
    cost->arg = t->arg;
    cost->tid = t->tid;

    // Measure function execution time
    start = clock();
    t->fn(t->arg);
    end = clock();

    // Store execution time
    cost->cost = end - start;

    // Update index
    cost->next = task_index;
    task_index = cost;
}

clock_t task_cost_get_from_task(struct task *t) {
    return task_cost_get_from_tid(t->tid);
}

clock_t task_cost_get_from_tid(int tid) {
    struct task_cost *curr = task_index;
    while (curr != NULL) {
        if (curr->tid == tid) {
            return curr->cost;
        }
        curr = curr->next;
    }

    return -1;
}
