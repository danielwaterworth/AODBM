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

#include <check.h>

#include "hash_test.h"
#include "data_test.h"
#include "rope_test.h"
#include "stack_test.h"
#include "rwlock_test.h"
#include "list_test.h"
#include "changeset_test.h"

int main(void) {
    int number_failed;
    Suite *s = suite_create("Main");
    
    suite_add_tcase(s, hash_test_case());
    suite_add_tcase(s, data_test_case());
    suite_add_tcase(s, rope_test_case());
    suite_add_tcase(s, stack_test_case());
    suite_add_tcase(s, rwlock_test_case());
    suite_add_tcase(s, list_test_case());
    suite_add_tcase(s, changeset_test_case());
    
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
