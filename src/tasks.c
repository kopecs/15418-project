/**
 * @file task_index.c
 */

#include <stdlib.h>
#include <time.h>

#include "tasks.h"

/** Index of task costs */
struct task_cost *task_index;

clock_t task_cost_measure(struct task *t) {
    clock_t start, end;

    clock_t c = task_cost_get_from_tid(t->tid);
    if (c != -1) {
        return c;
    }

    // TODO: check for fn and arg being the same but in differnt task

    // Allocate index node
    struct task_cost *cost = malloc(sizeof(struct task_cost));
    if (!cost) {
        return -1;
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

    return cost->cost;
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

struct task *task_create(void *(*fn)(void *), void *arg, int tid) {
    struct task *t = malloc(sizeof(struct task));
    if (!t) {
        return NULL;
    }

    t->tid = tid;
    t->fn = fn;
    t->arg = arg;
    t->cost = task_cost_measure(t);
    t->blocked = false;
    t->thread = -1;
    t->executing = false;
    t->done = false;
    pthread_mutex_init(&t->lock, NULL);

    return t;
}

