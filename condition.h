#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>
#include "monitor.h"

typedef struct {
    pthread_cond_t cond;
} Condition;

static inline void condition_init(Condition *c) {
    pthread_cond_init(&c->cond, NULL);
}

static inline void condition_destroy(Condition *c) {
    pthread_cond_destroy(&c->cond);
}

static inline void condition_wait(Condition *c, Monitor *m) {
    pthread_cond_wait(&c->cond, &m->mtx);
}

static inline void condition_signal(Condition *c) {
    pthread_cond_signal(&c->cond);
}

static inline void condition_broadcast(Condition *c) {
    pthread_cond_broadcast(&c->cond);
}

#endif