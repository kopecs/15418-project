/**
 * @file thr.c
 *
 * @brief Implementation of threads
 *
 * @authors cppierce jdropkin
 *
 * @date Spring 2021
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tasks.h"
#include "thr.h"

#define MAYBE_UNUSED __attribute__((unused))

/** Max number of threads */
#define MAX_THRS (32)
/** Number of os threads used */
#define NUM_OS_THRS (4)
/** Threshold for task cost to be stolen */
#define COST_THRESHOLD (3)

/**
 * @brief Queue for individual OS threads
 */
struct thread_queue {
    int tid;              /**< Thread id */
    int num_tasks;        /**< Number of tasks in queue */
    clock_t total_cost;   /**< Total cost of workers in the thread queue */
    pthread_mutex_t lock; /**< Queue lock */
    struct task *queue;   /**< Queue of tasks */
};

/**
 * @struct Global task queue
 * @brief Information about the work queue
 */
struct work_queue {
    int num_tasks;        /**< Number of tasks in queue */
    clock_t total_cost;   /**< Total cost of workers in the global queue */
    struct task *queue;   /**< Task queue */
    pthread_mutex_t lock; /**< Queue lock */
};

/** Array of worker threads */
pthread_t OS_THREADS[NUM_OS_THRS];

/** Boolean to keep track if execution has been marked as done */
bool DONE = false;

/** Global work queue of all tasks*/
struct work_queue *WQ;

/** Individual work thread queues */
struct thread_queue THREAD_QUEUES[NUM_OS_THRS];

/** @brief Find the max of two ints */
MAYBE_UNUSED static inline int max(int x, int y) { return x > y ? x : y; }

/** @brief Find the min of two ints */
MAYBE_UNUSED static inline int min(int x, int y) { return x < y ? x : y; }

/**
 * @brief Get task struct from task id
 * @param tid Task id
 * @return Task struct corresponding with tid
 */
MAYBE_UNUSED static inline struct task *get_task(int tid) {
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

/**
 * @brief Insert a task into the global queue
 *
 * This queue is sorted from highest cost to lowest cost
 *
 * This list uses fine grained hand-over-hand locking
 *
 * @param t The task to add
 */
static inline void work_queue_insert(struct task *t) {
    pthread_mutex_lock(&WQ->lock);
    struct task *prev = WQ->queue;
    struct task *curr = prev->next;

    // If list is empty
    if (prev == NULL) {
        WQ->queue = t;
        WQ->num_tasks = 1;
        WQ->total_cost = t->cost;
        pthread_mutex_unlock(&WQ->lock);
        return;
    }

    // Check if cost(t) > max cost in list
    if (t->cost > prev->cost) {
        t->next = prev;
        WQ->queue = t;

        // Update work queue
        WQ->num_tasks++;
        WQ->total_cost += t->cost;
        pthread_mutex_unlock(&WQ->lock);
        return;
    }

    // Lock individual node, unlock queue lock
    pthread_mutex_lock(&prev->lock);
    pthread_mutex_unlock(&WQ->lock);

    // Loop through to find position of t
    while (curr != NULL) {
        pthread_mutex_lock(&curr->lock);

        if (t->cost > curr->cost) {
            prev->next = t;
            t->next = curr;
        }

        pthread_mutex_unlock(&prev->lock);
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&curr->lock);

    // Update work queue
    pthread_mutex_lock(&WQ->lock);
    WQ->num_tasks++;
    WQ->total_cost += t->cost;
    pthread_mutex_unlock(&WQ->lock);
}

/**
 * @brief Pop a task from a thread queue
 * @param tq The queue to pop from
 * @return The popped task
 */
static struct task *task_pop(struct thread_queue *tq) {
    assert(tq != NULL);
    pthread_mutex_lock(&tq->lock);
    struct task *t;

    // Check if there are tasks to pop
    if (tq->num_tasks == 0) {
        pthread_mutex_unlock(&tq->lock);
        return NULL;
    }

    tq->num_tasks--;
    t = tq->queue;
    tq->queue = t->next;

    pthread_mutex_unlock(&tq->lock);
    return t;
}

/**
 * @brief Have a thread execute a task
 * @param tid Task id to execute
 */
void thr_execute(struct task *t) {
    // Execute the work
    t->executing = true;
    t->ret = t->fn(t->arg);
    t->done = true;

    // Remove task from the global work queue
    pthread_mutex_lock(&WQ->lock);
    struct task *curr = WQ->queue;
    while (curr->next != t) {
        curr = curr->next;
    }

    // Update the work queue
    curr->next = t->next;

    // TODO: remove task from local work queue

    pthread_mutex_unlock(&WQ->lock);
}

/**
 * @brief Find the thread queue with the highest total cost
 *
 * Note that this function may not be perfect as it does not lock the queues,
 * but it does not need to be perfect, as the overhead of locking is too
 * expensive for the benefit of finding the single most expensive queue
 *
 * @return Busiest queue
 */
static inline struct thread_queue *find_busiest_queue() {
    struct thread_queue *best_queue = &THREAD_QUEUES[0];
    clock_t best_cost = best_queue->total_cost;

    // Loop through queues and find one with highest total cost
    for (int i = 1; i < NUM_OS_THRS; i++) {
        struct thread_queue *curr = &THREAD_QUEUES[i];
        if (curr->total_cost > best_cost) {
            best_cost = curr->total_cost;
            best_queue = curr;
        }
    }

    return best_queue;
}

/**
 * @brief Insert a task into a task queue
 *
 * @pre t is locked
 *
 * @param tq The queue to insert into
 * @param t The task to insert
 * @return Success status
 */
void thread_queue_insert(struct thread_queue *tq, struct task *t) {
    pthread_mutex_lock(&tq->lock);

    t->next = tq->queue;
    tq->queue = t;

    tq->num_tasks++;
    tq->total_cost += t->cost;

    pthread_mutex_unlock(&tq->lock);
}

/**
 * @brief The to queue steals a task from the from queue
 *
 * This uses hand over hand locking
 *
 * @param from The list being stolen from
 * @param to The stealer
 * @return Success status
 */
int steal_tasks(struct thread_queue *from, struct thread_queue *to) {
    struct task *curr;
    struct task *prev;

    if (!from || !to) {
        return 0;
    }

    pthread_mutex_lock(&from->lock);
    if (from->queue == NULL) {
        pthread_mutex_unlock(&from->lock);
        return 0;
    }

    pthread_mutex_lock(&from->queue->lock);

    // Check if head element is above threshold
    if (from->queue->cost * COST_THRESHOLD >= from->total_cost) {
        curr = from->queue;

        // Update queue being stolen from
        from->queue = curr->next;
        from->num_tasks--;
        from->total_cost -= curr->cost;

        pthread_mutex_unlock(&from->lock);

        thread_queue_insert(to, curr);
        pthread_mutex_unlock(&curr->lock);

        return 1;
    }

    // From queue does not need to be locked as head element is locked
    pthread_mutex_unlock(&from->lock);

    curr = from->queue->next;
    prev = from->queue;

    while (curr != NULL) {
        pthread_mutex_lock(&curr->lock);

        // If curr > threshold or curr is the tail
        if (curr->cost * COST_THRESHOLD >= from->total_cost ||
            curr->next == NULL) {
            // Remove curr from list
            prev->next = curr->next;

            // Update stolen from list
            pthread_mutex_lock(&from->lock);
            from->num_tasks--;
            from->total_cost -= curr->cost;
            pthread_mutex_unlock(&from->lock);

            pthread_mutex_unlock(&prev->lock);

            // Insert curr to stealer
            thread_queue_insert(to, curr);
        }

        if (prev != NULL) {
            pthread_mutex_unlock(&prev->lock);
        }

        prev = curr;
        curr = curr->next;
    }

    return 1;
}

/**
 * @brief Worker requests tasks from global queue
 * @param tq The queue for the worker
 * @return Number of tasks added to the queue
 */
int request_tasks(struct thread_queue *tq) {
    int tasks_added = 0;
    int num_to_add;

    // TODO: fine grained locking
    pthread_mutex_lock(&WQ->lock);

    // If there are no tasks in the global queue, no work can be added
    if (WQ->num_tasks == 0) {
        pthread_mutex_unlock(&WQ->lock);
        return 0;
    }

    // Add at least one task, but take a proportional number of tasks to the
    // number of threads
    num_to_add = max(WQ->num_tasks / NUM_OS_THRS, 1);

    // pop task off of work queue
    struct task *t = WQ->queue;
    pthread_mutex_lock(&t->lock);
    WQ->queue = t->next;
    pthread_mutex_unlock(&t->lock);

    // add task to thread queue
    pthread_mutex_lock(&tq->lock);
    t->next = tq->queue;
    tq->queue = t;

    // Update status of task
    t->thread = tq->tid;
    pthread_mutex_unlock(&tq->lock);
    tasks_added++;

    pthread_mutex_unlock(&WQ->lock);

    // If there are no tasks in the global queue, steal a task from another
    // thread
    if (tasks_added == 0) {
        struct thread_queue *target = find_busiest_queue();

        // If the searcher is the busiest queue, that means all other queues are
        // empty too as the searcher must be empty to get here
        if (target == tq) {
            return 0;
        } else {
            tasks_added += steal_tasks(target, tq);
        }
    }

    return tasks_added;
}

/**
 * @brief Main loop for OS threads
 * @param arg Thread's queue
 */
void *worker(void *arg) {
    int sleep_time = 1;

    struct thread_queue *tq = (struct thread_queue *)arg;

    // Main execution loop
    while (1) {
        // Check if all work is done
        if (DONE) {
            break;
        }

        if (tq->num_tasks == 0) {
            // Check if there is work to take from the global queue
            if (request_tasks(tq) == 0) {
                // No work; sleep TODO: maybe add a convar
                sleep(sleep_time);
                sleep_time *= 2;
            } else {
                sleep_time = 1;
            }
        }

        // Get task from queue
        struct task *t = task_pop(tq);

        // Chceck if there is actual work (could have beeen stolen between
        // checking and popping)
        if (t == NULL) {
            continue;
        }

        // Execute task
        thr_execute(t);
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
        struct thread_queue *tq = &THREAD_QUEUES[i];

        // Initialize thread's queue
        tq->tid = i;
        tq->num_tasks = 0;
        pthread_mutex_init(&tq->lock, NULL);
        tq->queue = NULL;

        // Create worker thread
        pthread_create(&OS_THREADS[i], NULL, worker, &THREAD_QUEUES[i]);
    }
}

/**
 * @brief Add a task to be computed
 * @param fn The function to execute
 * @param arg Arguments to the function
 * @return The task's id
 */
int thr_add(void *(*fn)(void *), void *arg) {
    // Allocate the task
    struct task *t = task_create(fn, arg, WQ->num_tasks);

    // Insert into the global work queue
    work_queue_insert(t);

    return t->tid;
}

/**
 * @brief Wait for a task to complete
 * @param tid Task id to wait for
 * @param[out] ret The return value of the task
 * @return Only returns once the task returns
 */
void thr_wait(int tid, void **ret) {
    struct task *t = get_task(tid);

    // Wait for task to be done
    while (!t->done)
        ;

    // Update return value
    if (ret != NULL) {
        *ret = t->ret;
    }
}

/**
 * @brief Cleanup function
 */
void thr_finish() {
    // Update done status
    DONE = true;

    // Wait for workers to finish up
    for (int i = 0; i < NUM_OS_THRS; i++) {
        pthread_join(OS_THREADS[i], NULL);
    }
}
