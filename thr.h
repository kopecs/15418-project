/**
 * @file thr.h
 *
 * @brief Interface for threads
 *
 * @authors jdropkin cppierce
 *
 * @date Spring 2021
 */

#ifndef C0_THR_H
#define C0_THR_H

/**
 * @brief Add a task
 * @param fn The function to compute
 * @param vargp The parameter to the function
 * @return Thread id on success, -1 on fail
 */
int thr_add(void *fn, void *vargp);

/**
 * @brief Wait for a thread to complete
 * @param tid The thread to wait for
 */
void thr_wait(int tid);

#endif /* C0_THR_H */
