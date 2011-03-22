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

#include "changeset_test.h"
#include "aodbm.h"
#include "aodbm_data.h"

START_TEST (test_1) {
    aodbm_changeset set = aodbm_changeset_empty();
    aodbm_data *key = aodbm_data_from_str("hello");
    aodbm_data *val = aodbm_data_from_str("world");
    aodbm_changeset_add_modify(set, key, val);
    aodbm *db = aodbm_open("testdb", 0);
    aodbm_version ver = aodbm_apply_di(db, 0, set);
    fail_unless(ver != 0);
    aodbm_data *r_val = aodbm_get(db, ver, key);
    fail_unless(r_val != NULL, NULL);
    fail_unless(aodbm_data_eq(r_val, val), NULL);
    aodbm_free_data(key);
    aodbm_free_data(val);
    aodbm_free_data(r_val);
    aodbm_close(db);
} END_TEST

TCase *changeset_test_case() {
    TCase *tc = tcase_create("changeset");
    tcase_add_test(tc, test_1);
    return tc;
}
