/**
 * @file task_index.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tasks.h"

/** If costs need to be measured or if the user provides a cost */
const bool TIME_TASKS = false;

/** Index of task costs */
struct task_cost *task_index;

struct task_map {
    int task_id;
    struct task *t;
    struct task_map *next;
};

struct task_map *MAP;
static pthread_mutex_t TASK_MAP_LOCK = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Measure the cost of a task and insert it into the index
 * @param t The task to measure
 * @return The task cost
 */
clock_t task_cost_measure(struct task *t, clock_t given_cost) {
    clock_t start, end;

    clock_t c = task_cost_get_from_tid(t->tid);
    if (c != -1) {
        return c;
    }

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
    printf("[DBG] Measuring the cost of a task\n");

    // User has given a cost
    if (!TIME_TASKS) {
        cost->cost = given_cost;
        cost->next = task_index;
        task_index = cost;
        return cost->cost;
    }

    // Need to time task cost
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
struct task *task_create(void *(*fn)(void *), void *arg, int tid,
                         clock_t cost) {
    // Allocate task
    struct task *t = malloc(sizeof(struct task));
    if (!t) {
        return NULL;
    }

    // Initialize task
    t->tid = tid;
    t->fn = fn;
    t->arg = arg;
    t->cost = task_cost_measure(t, cost);
    t->blocked = false;
    t->thread = -1;
    t->executing = false;
    t->done = false;
    t->next = NULL;
    pthread_mutex_init(&t->lock, NULL);

    return t;
}

void task_map_add(int task_id, struct task *t) {
    struct task_map *tm = malloc(sizeof(struct task_map));
    if (!tm) {
        return;
    }

    pthread_mutex_lock(&TASK_MAP_LOCK);

    tm->task_id = task_id;
    tm->t = t;
    tm->next = MAP;
    MAP = tm;
    pthread_mutex_unlock(&TASK_MAP_LOCK);
}

/**
 * @brief Find a task map entry
 *
 * @pre The task map is locked
 *
 * @param task_id The task to find
 * @return The task map entry
 */
struct task *task_map_find(int task_id) {
    struct task_map *curr = MAP;

    while (curr != NULL) {
        if (curr->task_id == task_id) {
            return curr->t;
        }
        curr = curr->next;
    }

    // Couldn't find the task
    return NULL;
}
