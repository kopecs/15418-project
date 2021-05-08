/**
 * @file task_index.h
 */

#ifndef TASK_INDEX_H
#define TASK_INDEX_H

#include <stdbool.h>
#include <pthread.h>

struct task_cost {
    int tid;
    void *(*fn)(void *);
    void *arg;
    clock_t cost;
    struct task_cost *next;
};

struct task {
    int cost;              /**< Task cost */
    int tid;               /**< Task id */
    bool blocked;          /**< Blocked status */
    void *(*fn)(void *);   /**< Task function */
    void *arg;             /**< Task arguments */
    pthread_mutex_t lock;  /**< Lock for fine grained locking */
    struct task *next;     /**< Next task in queue */
};

void task_cost_measure(struct task *t);

clock_t task_cost_get_from_task(struct task *t);

clock_t task_cost_get_from_tid(int tid);

#endif /* TASK_INDEX_H */
