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

#include "stack_test.h"
#include "aodbm_stack.h"

START_TEST (test_1) {
    aodbm_stack *st = NULL;
    aodbm_stack_push(&st, (void *)10);
    fail_unless(st != NULL, NULL);
    fail_unless(aodbm_stack_pop(&st) == (void *)10, NULL);
    fail_unless(st == NULL, NULL);
    aodbm_stack_push(&st, (void *)10);
    aodbm_stack_push(&st, (void *)20);
    aodbm_stack_push(&st, (void *)30);
    fail_unless(aodbm_stack_pop(&st) == (void *)30, NULL);
    fail_unless(aodbm_stack_pop(&st) == (void *)20, NULL);
    fail_unless(aodbm_stack_pop(&st) == (void *)10, NULL);
    fail_unless(st == NULL, NULL);
} END_TEST

TCase *stack_test_case() {
    TCase *tc = tcase_create("stack");
    tcase_add_test(tc, test_1);
    return tc;
}
