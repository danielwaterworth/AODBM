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

#include "aodbm_changeset.h"
#include "aodbm_data.h"
#include "aodbm_error.h"

#define AODBM_MODIFY 1
#define AODBM_REMOVE 2

typedef struct {
    unsigned char type;
    aodbm_data *key;
    aodbm_data *val;
} change;

static change *create_remove_di(aodbm_data *key) {
    change *c = malloc(1 + sizeof(aodbm_data *));
    c->type = AODBM_REMOVE;
    c->key = key;
    return c;
}

static change *create_modify_di(aodbm_data *key, aodbm_data *val) {
    change *c = malloc(sizeof(change));
    c->type = AODBM_MODIFY;
    c->key = key;
    c->val = val;
    return c;
}

static void free_change(change *c) {
    switch (c->type) {
        case AODBM_REMOVE:
            break;
        case AODBM_MODIFY:
            aodbm_free_data(c->val);
            break;
        default:
            AODBM_CUSTOM_ERROR("unknown change type");
    };
    aodbm_free_data(c->key);
    free(c);
}

aodbm_changeset aodbm_changeset_empty() {
    aodbm_changeset result;
    result.list = aodbm_list_empty();
    return result;
}

void aodbm_changeset_add_modify_di(aodbm_changeset changeset,
                                   aodbm_data *key,
                                   aodbm_data *val) {
    change *c = create_modify_di(key, val);
    aodbm_list_push_back(changeset.list, c);
}

void aodbm_changeset_add_remove_di(aodbm_changeset changeset, aodbm_data *key) {
    change *c = create_remove_di(key);
    aodbm_list_push_back(changeset.list, c);
}

aodbm_changeset aodbm_changeset_merge_di(aodbm_changeset a, aodbm_changeset b) {
    aodbm_changeset result;
    result.list = aodbm_list_merge_di(a.list, b.list);
    /* TODO: remove unneeded changes */
    return result;
}

void aodbm_free_changeset(aodbm_changeset c) {
    while (!aodbm_list_is_empty(c.list)) {
        free_change(aodbm_list_pop_back(c.list));
    }
    aodbm_free_list(c.list);
}
