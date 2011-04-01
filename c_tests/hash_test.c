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

#include "hash_test.h"
#include "aodbm_hash.h"

unsigned int hash_int(void *item) {
    return (unsigned int)item;
}

bool cmp_int(void *a, void *b) {
    return a == b;
}

START_TEST (test_1) {
    aodbm_hash *ht = aodbm_new_hash(0, hash_int, cmp_int);
    aodbm_hash_insert(ht, (void *)10);
    fail_unless(aodbm_hash_get(ht, (void *)10) == (void *)10, NULL);
    aodbm_free_hash(ht);
} END_TEST

TCase *hash_test_case() {
    TCase *tc = tcase_create("hash table");
    tcase_add_test(tc, test_1);
    return tc;
}
