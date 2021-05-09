/**
 * @file task_index.c
 */

#include <stdlib.h>
#include <time.h>

#include "tasks.h"

/** Index of task costs */
struct task_cost *task_index;

/**
 * @brief Measure the cost of a task and insert it into the index
 * @param t The task to measure
 * @return The task cost
 */
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

/**
 * @brief Get the cost from a task
 * @param t The task
 * @return The cost, -1 if the task has not been measured
 */
clock_t task_cost_get_from_task(struct task *t) {
    return task_cost_get_from_tid(t->tid);
}

/**
 * @brief Get a tasks's cost from it's task id
 * @param tid The task id
 * @return The task's cost, -1 if the task has not been measured
 */
clock_t task_cost_get_from_tid(int tid) {
    struct task_cost *curr = task_index;

    // Search until matching tid is found
    while (curr != NULL) {
        if (curr->tid == tid) {
            return curr->cost;
        }
        curr = curr->next;
    }

    // Task not found
    return -1;
}

/**
 * @brief Create a task
 * @param fn The task's function
 * @param arg The task's argument
 * @param tid The task's id
 * @return Task struct
 */
struct task *task_create(void *(*fn)(void *), void *arg, int tid) {
    // Allocate task
    struct task *t = malloc(sizeof(struct task));
    if (!t) {
        return NULL;
    }

    // Initialize task
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
