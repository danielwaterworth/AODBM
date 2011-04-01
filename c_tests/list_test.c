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

#include "list_test.h"
#include "aodbm_list.h"

START_TEST (test_1) {
    aodbm_list *list = aodbm_list_empty();
    aodbm_list_push_back(list, (void *)10);
    fail_unless(aodbm_list_pop_back(list) == (void *)10, NULL);
    aodbm_free_list(list);
} END_TEST

START_TEST (test_2) {
    aodbm_list *list = aodbm_list_empty();
    aodbm_list_push_front(list, (void *)10);
    aodbm_list_push_front(list, (void *)20);
    fail_unless(aodbm_list_pop_back(list) == (void *)10, NULL);
    fail_unless(aodbm_list_pop_front(list) == (void *)20, NULL);
    aodbm_free_list(list);
} END_TEST

START_TEST (test_3) {
    aodbm_list *list = aodbm_list_empty();
    aodbm_list_push_back(list, (void *)10);
    aodbm_list_push_back(list, (void *)20);
    aodbm_list_push_back(list, (void *)30);
    
    aodbm_list_iterator *it = aodbm_list_begin(list);
    fail_unless(aodbm_list_iterator_get(it) == (void *)10, NULL);
    fail_unless(aodbm_list_iterator_is_begin(it), NULL);
    fail_if(aodbm_list_iterator_is_end(it), NULL);
    aodbm_list_iterator_next(it);
    fail_unless(aodbm_list_iterator_get(it) == (void *)20, NULL);
    fail_if(aodbm_list_iterator_is_begin(it), NULL);
    fail_if(aodbm_list_iterator_is_end(it), NULL);
    aodbm_list_iterator_next(it);
    fail_unless(aodbm_list_iterator_get(it) == (void *)30, NULL);
    fail_if(aodbm_list_iterator_is_begin(it), NULL);
    fail_unless(aodbm_list_iterator_is_end(it), NULL);
    aodbm_free_list_iterator(it);
    
    aodbm_list_pop_back(list);
    aodbm_list_pop_back(list);
    aodbm_list_pop_back(list);
    aodbm_free_list(list);
} END_TEST

TCase *list_test_case() {
    TCase *tc = tcase_create("list");
    tcase_add_test(tc, test_1);
    tcase_add_test(tc, test_2);
    tcase_add_test(tc, test_3);
    return tc;
}
