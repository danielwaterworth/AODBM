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

#ifndef AODBM_LIST_H
#define AODBM_LIST_H

#include "stdbool.h"
#include "stdlib.h"

struct aodbm_list;
typedef struct aodbm_list aodbm_list;

struct aodbm_list_iterator;
typedef struct aodbm_list_iterator aodbm_list_iterator;

aodbm_list *aodbm_list_empty();
void aodbm_free_list(aodbm_list *);

void aodbm_list_push_front(aodbm_list *, void *);
void aodbm_list_push_back(aodbm_list *, void *);
void *aodbm_list_pop_front(aodbm_list *);
void *aodbm_list_pop_back(aodbm_list *);

bool aodbm_list_is_empty(aodbm_list *);
size_t aodbm_list_length(aodbm_list *);

aodbm_list *aodbm_list_merge_di(aodbm_list *, aodbm_list *);

void aodbm_list_insert(aodbm_list_iterator *, void *);
void aodbm_list_remove(aodbm_list_iterator *);

aodbm_list_iterator *aodbm_list_begin(aodbm_list *);
aodbm_list_iterator *aodbm_list_end(aodbm_list *);

void aodbm_list_iterator_next(aodbm_list_iterator *);
void aodbm_list_iterator_prev(aodbm_list_iterator *);
bool aodbm_list_iterator_is_begin(aodbm_list_iterator *);
bool aodbm_list_iterator_is_end(aodbm_list_iterator *);
void *aodbm_list_iterator_get(aodbm_list_iterator *);

#endif
