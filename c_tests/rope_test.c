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

#include "rope_test.h"
#include "aodbm_rope.h"

START_TEST (test_1) {

} END_TEST

TCase *rope_test_case() {
    TCase *tc = tcase_create("rope");
    tcase_add_test(tc, test_1);
    return tc;
}
