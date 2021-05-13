/**
 * @file main.c
 */

#include <stdio.h>
#include <unistd.h>

#include "thr.h"

void *f(void *arg) {
    (void)arg;
    sleep(2);
    printf("Hello world\n");
    return NULL;
}

void *g(void *arg) {
    (void)arg;
    printf("Hello from g\n");
    return NULL;
}

void *h(void *arg) {
    int i = (int)arg;
    printf("%d\n", i);
    usleep(5000);
    return NULL;
}

int main(void) {
    thr_init();
    int num_threads = 100;
    int tids[num_threads];
    tids[0] = thr_add(f, NULL, 100);
    for (int i = 1; i < num_threads; i++) {
        tids[i] = thr_add(h, (void *)i, 10);
    }

    thr_start();
    for (int i = 0; i < num_threads; i++) {
        thr_wait(tids[i], NULL);
    }

    /*int tidf = thr_add(f, NULL, 100);
    int tidg = thr_add(g, NULL, 1);
    thr_wait(tidf, NULL);
    thr_wait(tidg, NULL);*/
    thr_finish();
    return 0;
}
