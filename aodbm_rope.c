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

#include <stdlib.h>
#include <string.h>

#include "aodbm.h"

struct aodbm_rope_node {
    aodbm_data *dat;
    struct aodbm_rope_node *next;
};

typedef struct aodbm_rope_node aodbm_rope_node;

struct aodbm_rope {
    aodbm_rope_node *first;
    aodbm_rope_node *last;
};

aodbm_rope_node *aodbm_data_to_rope_node_di(aodbm_data *dat) {
    aodbm_rope_node *node = malloc(sizeof(aodbm_rope_node));
    node->dat = aodbm_data_dup(dat);
    node->next = NULL;
    return node;
}

aodbm_rope_node *aodbm_data_to_rope_node(aodbm_data *dat) {
    return aodbm_data_to_rope_node_di(aodbm_data_dup(dat));
}

aodbm_rope *aodbm_data_to_rope_di(aodbm_data *dat) {
    aodbm_rope *result = malloc(sizeof(aodbm_rope));
    aodbm_rope_node *node = aodbm_data_to_rope_node_di(dat);
    result->first = node;
    result->last = node;
    return result;
}

aodbm_rope *aodbm_data_to_rope(aodbm_data *dat) {
    return aodbm_data_to_rope_di(aodbm_data_dup(dat));
}

aodbm_rope *aodbm_rope_empty() {
    return aodbm_data_to_rope_di(aodbm_data_empty());
}

size_t aodbm_rope_size(aodbm_rope *rope) {
    aodbm_rope_node *it;
    size_t sz = 0;
    for (it = rope->first; it != NULL; it = it->next) {
        sz += it->dat->sz;
    }
    return sz;
}

aodbm_data *aodbm_rope_to_data(aodbm_rope *rope) {
    size_t sz = aodbm_rope_size(rope);
    
    aodbm_data *result = malloc(sizeof(aodbm_data));
    result->dat = malloc(sz);
    result->sz = sz;
    
    size_t pos = 0;
    aodbm_rope_node *it;
    for (it = rope->first; it != NULL; it = it->next) {
        memcpy(&result->dat[pos], it->dat->dat, it->dat->sz);
        pos += it->dat->sz;
    }
    
    return result;
}

aodbm_data *aodbm_rope_to_data_di(aodbm_rope *rope) {
    aodbm_data *dat = aodbm_rope_to_data(rope);
    aodbm_free_rope(rope);
    return dat;
}

void aodbm_free_rope(aodbm_rope *rope) {
    aodbm_rope_node *it;
    for (it = rope->first; it != NULL;) {
        aodbm_free_data(it->dat);
        aodbm_rope_node *next = it->next;
        free(it);
        it = next;
    }
    free(rope);
}

void aodbm_rope_append_di(aodbm_rope *rope, aodbm_data *dat) {
    aodbm_rope_node *node = aodbm_data_to_rope_node_di(dat);
    rope->last->next = node;
    rope->last = node;
}

void aodbm_rope_prepend_di(aodbm_data *dat, aodbm_rope *rope) {
    aodbm_rope_node *node = aodbm_data_to_rope_node_di(dat);
    node->next = rope->first;
    rope->first = node;
}

void aodbm_rope_append(aodbm_rope *rope, aodbm_data *dat) {
    aodbm_rope_append_di(rope, aodbm_data_dup(dat));
}

void aodbm_rope_prepend(aodbm_data *dat, aodbm_rope *rope) {
    aodbm_rope_prepend_di(aodbm_data_dup(dat), rope);
}

aodbm_rope *aodbm_rope_merge_di(aodbm_rope *a, aodbm_rope *b) {
    a->last->next = b->first;
    a->last = b->last;
    free(b);
    return a;
}

aodbm_rope *aodbm_data2_to_rope_di(aodbm_data *a, aodbm_data *b) {
    aodbm_rope *dat = aodbm_data_to_rope_di(a);
    aodbm_rope_append_di(dat, b);
    return dat;
}

aodbm_rope *aodbm_data2_to_rope(aodbm_data *a, aodbm_data *b) {
    return aodbm_data2_to_rope_di(aodbm_data_dup(a), aodbm_data_dup(b));
}
