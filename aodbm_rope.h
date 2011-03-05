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

#ifndef AODBM_ROPE_H
#define AODBM_ROPE_H

/* this type is used to make merging aodbm_data objects cheaper */
/* implementation note:
     internally it is just a list of aodbm_data objects,
     rather than the more usual tree of strings */
struct aodbm_rope;
typedef struct aodbm_rope aodbm_rope;

/* aodbm_rope functions */
aodbm_rope *aodbm_rope_empty();
aodbm_rope *aodbm_data_to_rope_di(aodbm_data *);
aodbm_rope *aodbm_data_to_rope(aodbm_data *);
aodbm_rope *aodbm_data2_to_rope_di(aodbm_data *, aodbm_data *);
aodbm_rope *aodbm_data2_to_rope(aodbm_data *, aodbm_data *);
size_t aodbm_rope_size(aodbm_rope *);
aodbm_data *aodbm_rope_to_data(aodbm_rope *);
aodbm_data *aodbm_rope_to_data_di(aodbm_rope *);
void aodbm_free_rope(aodbm_rope *);

/* the aodbm_data object is destroyed */
void aodbm_rope_append_di(aodbm_rope *, aodbm_data *);
void aodbm_rope_prepend_di(aodbm_data *, aodbm_rope *);

void aodbm_rope_append(aodbm_rope *, aodbm_data *);
void aodbm_rope_prepend(aodbm_data *, aodbm_rope *);
aodbm_rope *aodbm_rope_merge_di(aodbm_rope *, aodbm_rope *);

void aodbm_print_rope(aodbm_rope *);

#endif
