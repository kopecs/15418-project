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
    clock_t cost;          /**< Task cost */
    int tid;               /**< Task id */
    bool blocked;          /**< Blocked status */
    void *(*fn)(void *);   /**< Task function */
    void *arg;             /**< Task arguments */
    int thread;            /**< Thread the task is assigned to (-1 if unassigned) */
    bool executing;        /**< If the task has started executing */
    void *ret;             /**< Task's return value */
    bool done;             /**< If the task has finished executing */
    pthread_mutex_t lock;  /**< Lock for fine grained locking */
    struct task *next;     /**< Next task in queue */
};

clock_t task_cost_measure(struct task *t);

clock_t task_cost_get_from_task(struct task *t);

clock_t task_cost_get_from_tid(int tid);

struct task *task_create(void *(*fn)(void *), void *arg, int tid);

#endif /* TASK_INDEX_H */
