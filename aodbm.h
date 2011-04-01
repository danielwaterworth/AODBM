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

/* bool __sync_bool_compare_and_swap (type *ptr, type oldval, type newval) */
/* TODO: better support for trees as values */

#ifndef AODBM
#define AODBM

#include "aodbm_changeset.h"

#include <stdbool.h>
#include <stdint.h>

/* the type of the database object */
struct aodbm;

/* the main type for data */
struct aodbm_data {
    char *dat;
    size_t sz;
};

typedef struct aodbm aodbm;
typedef struct aodbm_data aodbm_data;

typedef uint64_t aodbm_version;

aodbm *aodbm_open(const char *, int);
void aodbm_close(aodbm *);

aodbm_version aodbm_current(aodbm *);
bool aodbm_commit(aodbm *, aodbm_version);

bool aodbm_has(aodbm *, aodbm_version, aodbm_data *);
aodbm_version aodbm_set(aodbm *, aodbm_version, aodbm_data *, aodbm_data *);
aodbm_data *aodbm_get(aodbm *, aodbm_version, aodbm_data *);
aodbm_version aodbm_del(aodbm *, aodbm_version, aodbm_data *);

bool aodbm_is_based_on(aodbm *, aodbm_version, aodbm_version);
aodbm_version aodbm_previous_version(aodbm *, aodbm_version);
aodbm_version aodbm_common_ancestor(aodbm *, aodbm_version, aodbm_version);

aodbm_changeset aodbm_diff_prev(aodbm *, aodbm_version);
aodbm_changeset aodbm_diff_prev_rev(aodbm *, aodbm_version);
aodbm_changeset aodbm_diff(aodbm *, aodbm_version, aodbm_version);
aodbm_version aodbm_apply(aodbm *, aodbm_version, aodbm_changeset);
aodbm_version aodbm_apply_di(aodbm *, aodbm_version, aodbm_changeset);
aodbm_version aodbm_merge(aodbm *, aodbm_version, aodbm_version);

/* iteration API */
struct aodbm_iterator;
typedef struct aodbm_iterator aodbm_iterator;

struct aodbm_record {
    aodbm_data *key;
    aodbm_data *val;
};

typedef struct aodbm_record aodbm_record;

aodbm_iterator *aodbm_new_iterator(aodbm *, aodbm_version);
aodbm_iterator *aodbm_iterate_from(aodbm *, aodbm_version, aodbm_data *);
aodbm_record aodbm_iterator_next(aodbm *, aodbm_iterator *);
void aodbm_iterator_goto(aodbm *, aodbm_iterator *it, aodbm_data *);
void aodbm_free_iterator(aodbm_iterator *);

void aodbm_free_data(aodbm_data *);

#endif
