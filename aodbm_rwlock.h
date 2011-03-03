/*
    An implementation of a rwlock that won't stave the writers.
*/

#ifndef AODBM_RWLOCK_H
#define AODBM_RWLOCK_H

#include "pthread.h"
#include "stdint.h"
#include "stdbool.h"

struct aodbm_rwlock_t {
    pthread_mutex_t mut;
    pthread_cond_t cnd;
    volatile size_t readers;
    volatile size_t writers_waiting;
    volatile bool is_writing;
};

typedef struct aodbm_rwlock_t aodbm_rwlock_t;

void aodbm_rwlock_init(aodbm_rwlock_t *);
void aodbm_rwlock_destroy(aodbm_rwlock_t *);

void aodbm_rwlock_rdlock(aodbm_rwlock_t *);
void aodbm_rwlock_wrlock(aodbm_rwlock_t *);
void aodbm_rwlock_unlock(aodbm_rwlock_t *);

bool aodbm_rwlock_tryrdlock(aodbm_rwlock_t *);
bool aodbm_rwlock_trywrlock(aodbm_rwlock_t *);

#endif
