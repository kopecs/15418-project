/**
 * @file main.c
 */

#include <stdio.h>

#include "thr.h"

void *f(void *arg) {
    (void)arg;
    printf("Hello world\n");
    return NULL;
}

int main(void) {
    thr_init();
    int tid = thr_add(f, NULL);
    thr_wait(tid, NULL);
    thr_finish();
    return 0;
}
