/**
 * @file main.c
 */

#include <stdio.h>
#include <unistd.h>

#include "thr.h"

void *f(void *arg) {
    (void)arg;
    printf("Hello world\n");
    //sleep(1);
    return NULL;
}

void *g(void *arg) {
    (void)arg;
    printf("Hello from g\n");
    return NULL;
}

int main(void) {
    thr_init();
    int tidf = thr_add(f, NULL);
    int tidg = thr_add(g, NULL);
    thr_wait(tidf, NULL);
    thr_wait(tidg, NULL);
    thr_finish();
    return 0;
}
