/*  
    aodbm - Append Only Database Manager
    Copyright (C) 2011 Daniel Waterworth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rwlock_test.h"
#include "aodbm_rwlock.h"

#include "stdbool.h"
#include "pthread.h"

typedef struct {
    aodbm_rwlock_t *lock;
    volatile bool *locked;
} lock_data;

static void *obtain_wrlock(void *ptr) {
    lock_data *dat = (lock_data *)ptr;
    
    fail_unless(*dat->locked);
    aodbm_rwlock_wrlock(dat->lock);
    fail_unless(*dat->locked == false);
    aodbm_rwlock_unlock(dat->lock);
    
    return NULL;
}

START_TEST (test_1) {
    pthread_t thread;
    aodbm_rwlock_t lock;
    aodbm_rwlock_init(&lock);
    
    aodbm_rwlock_rdlock(&lock);
    fail_unless(aodbm_rwlock_tryrdlock(&lock));
    aodbm_rwlock_unlock(&lock);
    
    volatile bool locked = true;
    
    lock_data dat;
    dat.lock = &lock;
    dat.locked = &locked;
    
    fail_unless(aodbm_rwlock_trywrlock(&lock) == false);
    
    pthread_create(&thread, NULL, obtain_wrlock, (void *)&dat);
    locked = false;
    aodbm_rwlock_unlock(&lock);
    
    aodbm_rwlock_destroy(&lock);
} END_TEST

TCase *rwlock_test_case() {
    TCase *tc = tcase_create("rwlock");
    tcase_add_test(tc, test_1);
    return tc;
}
