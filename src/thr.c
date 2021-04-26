/**
 * @file thr.c
 *
 * @brief Implementation of threads
 *
 * @authors cppierce jdropkin
 *
 * @date Spring 2021
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "thr.h"

#define MAX_THRS (32)

/**
 * @struct Thread info
 * @brief Information about an individual thread
 */
struct thread_info {
    int tid;
    void *(*fn)(void *);
    void *arg;
    struct thread_info *next;
};

/**
 * @struct Work queue
 * @brief Information about the work queue
 */
struct work_queue {
    int num_threads;
    struct thread_info *queue;
    pthread_mutex_t lock;
};

struct work_queue *WQ;

static struct thread_info *get_thr(int tid) {
    pthread_mutex_lock(&WQ->lock);
    struct thread_info *curr = WQ->queue;
    while (curr != NULL) {
        if (curr->tid == tid) {
            break;
        }
    }

    pthread_mutex_unlock(&WQ->lock);
    return curr;
}

void thr_init(void) {
    WQ = malloc(sizeof(struct work_queue));
    if (!WQ) {
        exit(-1);
    }

    WQ->num_threads = 0;
    WQ->queue = NULL;
    pthread_mutex_init(&WQ->lock, NULL);
}

int thr_add(void *(*fn)(void *), void *arg) {
    pthread_mutex_lock(&WQ->lock);
    struct thread_info *t = malloc(sizeof(struct thread_info));
    if (!t) {
        pthread_mutex_unlock(&WQ->lock);
        return -1;
    }

    WQ->num_threads++;

    t->tid = WQ->num_threads;
    t->fn = fn;
    t->arg = arg;
    t->next = WQ->queue;

    WQ->queue = t;
    pthread_mutex_unlock(&WQ->lock);
    return t->tid;
}

void thr_wait(int tid) {
    return;
}

void thr_execute(int tid) {
    struct thread_info *t = get_thr(tid);
    t->fn(t->arg);
    // TODO: update queue
}
