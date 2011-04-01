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

#ifndef AODBM_DATA_H
#define AODBM_DATA_H

#include "stdint.h"
#include "stdbool.h"

#include "aodbm.h"

/* NOTE: functions ending in di have destructive input */

/* aodbm_data functions */
aodbm_data *aodbm_construct_data(const char *, size_t);
aodbm_data *aodbm_data_dup(aodbm_data *);
aodbm_data *aodbm_data_from_str(const char *);
aodbm_data *aodbm_data_from_32(uint32_t);
aodbm_data *aodbm_data_from_64(uint64_t);
aodbm_data *aodbm_cat_data(aodbm_data *, aodbm_data *);
aodbm_data *aodbm_cat_data_di(aodbm_data *, aodbm_data *);
aodbm_data *aodbm_data_empty();
aodbm_data *aodbm_data_dup(aodbm_data *);

bool aodbm_data_lt(aodbm_data *, aodbm_data *);
bool aodbm_data_gt(aodbm_data *, aodbm_data *);
bool aodbm_data_le(aodbm_data *, aodbm_data *);
bool aodbm_data_ge(aodbm_data *, aodbm_data *);
bool aodbm_data_eq(aodbm_data *, aodbm_data *);
int aodbm_data_cmp(aodbm_data *, aodbm_data *);

/* data printing */
void aodbm_print_data(aodbm_data *);

#endif
