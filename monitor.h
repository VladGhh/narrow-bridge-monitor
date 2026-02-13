#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t mtx;
} Monitor;

static inline void monitor_init(Monitor *m) {
    pthread_mutex_init(&m->mtx, NULL);
}

static inline void monitor_destroy(Monitor *m) {
    pthread_mutex_destroy(&m->mtx);
}

static inline void monitor_enter(Monitor *m) {
    pthread_mutex_lock(&m->mtx);
}

static inline void monitor_exit(Monitor *m) {
    pthread_mutex_unlock(&m->mtx);
}

#endif