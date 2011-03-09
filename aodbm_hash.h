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

#ifndef AODBM_HASH_H
#define AODBM_HASH_H

struct aodbm_hash;
typedef struct aodbm_hash aodbm_hash;

aodbm_hash *aodbm_new_hash
    (unsigned int, unsigned int (*)(void *), bool (*)(void *, void *));
void aodbm_hash_insert(aodbm_hash *, void *);
void aodbm_hash_del(aodbm_hash *, void *);
void *aodbm_hash_get(aodbm_hash *, void *);
void aodbm_free_hash(aodbm_hash *);

#endif