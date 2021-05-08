/**
 * @file thr.c
 *
 * @brief Implementation of threads
 *
 * @authors cppierce jdropkin
 *
 * @date Spring 2021
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "thr.h"
#include "task_index.h"

#define MAX_THRS (32)
#define NUM_OS_THRS (4)

pthread_t OS_THREADS[NUM_OS_THRS];

bool DONE = false;

/**
 * @brief Queue for individual OS threads
 */
struct thread_queue {
    int tid;             /**< Thread id */
    int num_tasks;       /**< Number of tasks in queue */
    struct task *queue;  /**< Queue of tasks */
};

/**
 * @struct Global task queue
 * @brief Information about the work queue
 */
struct work_queue {
    int num_tasks;         /**< Number of tasks in queue */
    struct task *queue;    /**< Task queue */
    pthread_mutex_t lock;  /**< Queue lock */
};

struct work_queue *WQ;

/**
 * @brief Get task struct from task id
 * @param tid Task id
 * @return Task struct corresponding with tid
 */
static struct task *get_task(int tid) {
    pthread_mutex_lock(&WQ->lock);

    // Loop through global tasks to find individual one
    // TODO: maybe change this if tasks are removed from global queue
    struct task *curr = WQ->queue;
    while (curr != NULL) {
        if (curr->tid == tid) {
            break;
        }
    }

    pthread_mutex_unlock(&WQ->lock);
    return curr;
}

static struct task *task_pop(struct thread_queue *tq) {
    assert(tq != NULL);
    struct task *t;

    if (tq->num_tasks == 0) {
        return NULL;
    }

    tq->num_tasks--;
    t = tq->queue;
    tq->queue = t->next;

    return t;
}

/**
 * @brief Main loop for OS threads
 * @param arg Thread's queue
 */
void *worker(void *arg) {
    struct thread_queue *tq = (struct thread_queue *)arg;

    // Main execution loop
    while (1) {
        // Check if all work is done
        if (DONE) {
            break;
        }

        if (tq->num_tasks == 0) {
            // TODO: sleep, maybe on a convar
        }

        // Get task from queue
        struct task *t = task_pop(tq);

        // Execute task
        t->fn(t->arg);

        continue;
    }

    return NULL;
}

/**
 * @brief Initialize threading
 */
void thr_init(void) {
    // Allocate global queue
    WQ = malloc(sizeof(struct work_queue));
    if (!WQ) {
        exit(-1);
    }

    // Initialize global queue
    WQ->num_tasks = 0;
    WQ->queue = NULL;
    pthread_mutex_init(&WQ->lock, NULL);

    // TODO: allocate individual queues

    // Create worker threads
    for (int i = 0; i < NUM_OS_THRS; i++) {
        pthread_create(&OS_THREADS[i], NULL, worker, NULL);
    }
}

/**
 * @brief Add a task to be computed
 * @param fn The function to execute
 * @param arg Arguments to the function
 * @return The task's id
 */
int thr_add(void *(*fn)(void *), void *arg) {
    pthread_mutex_lock(&WQ->lock);

    // Create new task
    struct task *t = malloc(sizeof(struct task));
    if (!t) {
        pthread_mutex_unlock(&WQ->lock);
        return -1;
    }

    // initialize task
    t->tid = WQ->num_tasks;
    t->fn = fn;
    t->arg = arg;
    t->next = WQ->queue;
    pthread_mutex_init(&t->lock, NULL);

    // Update global queue
    WQ->num_tasks++;
    WQ->queue = t;
    pthread_mutex_unlock(&WQ->lock);

    return t->tid;
}

/**
 * @brief Wait for a task to complete
 * @param tid Task id to wait for
 * @return Only returns once the task returns
 */
void thr_wait(int tid) {
    (void) tid;
    return;
}

/**
 * @brief Have a thread execute a task
 * @param tid Task id to execute
 */
void thr_execute(int tid) {
    // Get the thread struct
    struct task *t = get_task(tid);

    // Execute the work
    t->fn(t->arg);

    // Update the work queue
    pthread_mutex_lock(&WQ->lock);
    struct task *curr = WQ->queue;
    while (curr->next != t) {
        curr = curr->next;
    }

    curr->next = t->next;
    pthread_mutex_unlock(&WQ->lock);
}

/**
 * @brief Cleanup function
 */
void thr_finish() {
    DONE = true;
}

