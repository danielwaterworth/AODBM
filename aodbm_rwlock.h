/*  
    Copyright (C) 2011 aodbm authors,
    
    This file is part of aodbm.
    
    aodbm is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aodbm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
