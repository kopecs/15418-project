/**
 * @file thr.h
 *
 * @brief Interface for threads
 *
 * @authors cppierce jdropkin
 *
 * @date Spring 2021
 */

#ifndef C0_THR_H
#define C0_THR_H

#include <time.h>

/**
 * @brief Initialize threads
 */
void thr_init(void);

/**
 * @brief Add a task
 * @param fn The function to compute
 * @param vargp The parameter to the function
 * @return Thread id on success, -1 on fail
 */
int thr_add(void *(*fn)(void *), void *arg, clock_t cost);

/**
 * @brief Start executing threads
 */
void thr_start();

/**
 * @brief Wait for a thread to complete
 * @param tid The thread to wait for
 */
void thr_wait(int tid, void **ret);

/**
 * @brief Cleanup threads
 */
void thr_finish(void);

#endif /* C0_THR_H */
