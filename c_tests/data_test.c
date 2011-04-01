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

#include "data_test.h"
#include "aodbm_data.h"

START_TEST (test_1) {
    aodbm_data *a = aodbm_construct_data("hello", 5);
    fail_unless(a->sz == 5);
    fail_unless(memcmp("hello", a->dat, 5) == 0);
    
    aodbm_data *b = aodbm_data_dup(a);
    fail_unless(b->sz == 5);
    fail_unless(memcmp("hello", b->dat, 5) == 0);
    
    fail_unless(aodbm_data_eq(a, b));
    fail_unless(aodbm_data_cmp(a, b) == 0);
    aodbm_free_data(a);
    
    a = aodbm_data_from_str("hello");
    fail_unless(a->sz == 5);
    fail_unless(memcmp("hello", a->dat, 5) == 0);
    
    aodbm_free_data(a);
    aodbm_free_data(b);
    
    a = aodbm_data_empty();
    b = aodbm_data_empty();
    fail_unless(aodbm_data_eq(a, b));
    
    aodbm_free_data(a);
    aodbm_free_data(b);
} END_TEST

TCase *data_test_case() {
    TCase *tc = tcase_create("data");
    tcase_add_test(tc, test_1);
    return tc;
}
