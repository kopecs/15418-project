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

void *h(void *arg) {
    int i = (int)arg;
    printf("%d\n", i);
    return NULL;
}

int main(void) {
    thr_init();
    int num_threads = 16;
    int tids[num_threads];
    for (int i = 0; i < num_threads; i++) {
        tids[i] = thr_add(h, (void *)i);
    }

    for (int i = 0; i < num_threads; i++) {
        thr_wait(tids[i], NULL);
    }

    /*int tidf = thr_add(f, NULL);
    int tidg = thr_add(g, NULL);
    thr_wait(tidf, NULL);
    thr_wait(tidg, NULL);*/
    thr_finish();
    return 0;
}
