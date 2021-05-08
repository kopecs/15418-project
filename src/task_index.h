/**
 * @file task_index.h
 */

#ifndef TASK_INDEX_H
#define TASK_INDEX_H

struct task_cost {
    void *(*fn)(void *);
    void *arg;
    int cost;
    struct task_cost *next;
}

void task_cost_measure(struct task *t);

int task_cost_get_from_task(struct task *t);

int task_cost_get_from_tid(int tid);

#endif /* TASK_INDEX_H */
