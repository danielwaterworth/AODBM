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

#define AODBM_HASH_BUCKETS 5

#include "aodbm_hash.h"

#include "stdlib.h"

struct aodbm_hash {
    void **data;
    unsigned int sz;
    unsigned int (*hash_function)(void *);
    bool (*eq)(void *, void *);
};

static uint32_t hash(uint32_t a){
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}

aodbm_hash *aodbm_new_hash(unsigned int sz,
                           unsigned int (*hash_function)(void *),
                           bool (*eq)(void *, void *)) {
    if (sz == 0) {
        sz = 16;
    }
    aodbm_hash *hash = malloc(sizeof(aodbm_hash));
    hash->sz = sz;
    hash->hash_function = hash_function;
    hash->eq = eq;
    hash->data = malloc(sizeof(void *) * sz);
    unsigned int i;
    for (i = 0; i < sz; ++i) {
        hash->data[i] = NULL;
    }
    return hash;
}

void aodbm_hash_insert(aodbm_hash *ht, void *val) {
    unsigned int key = ht->hash_function(val);
    unsigned int orig_key = key;
    unsigned int i;
    for (i = 0; i < AODBM_HASH_BUCKETS; ++i) {
        void **b = &ht->data[key % ht->sz];
        if (*b == NULL) {
            *b = val;
            return;
        }
        key = hash(key);
    }
    void **old = ht->data;
    ht->data = malloc(sizeof(void *) * ht->sz * 2);
    unsigned int old_sz = ht->sz;
    ht->sz = ht->sz * 2;
    for (i = 0; i < ht->sz; ++i) {
        ht->data[i] = NULL;
    }
    for (i = 0; i < old_sz; ++i) {
        void *b = old[i];
        if (b != NULL) {
            aodbm_hash_insert(ht, b);
        }
    }
    free(old);
}

void aodbm_hash_del(aodbm_hash *ht, void *data) {
    unsigned int key = ht->hash_function(data);
    unsigned int orig_key = key;
    unsigned int i;
    for (i = 0; i < AODBM_HASH_BUCKETS; ++i) {
        void **b = &ht->data[key % ht->sz];
        if (ht->eq(data, *b)) {
            *b = NULL;
            return;
        }
        key = hash(key);
    }
}

void *aodbm_hash_get(aodbm_hash *ht, void *data) {
    unsigned int key = ht->hash_function(data);
    unsigned int orig_key = key;
    unsigned int i;
    for (i = 0; i < AODBM_HASH_BUCKETS; ++i) {
        void *b = ht->data[key % ht->sz];
        if (ht->eq(data, b)) {
            return b;
        }
        key = hash(key);
    }
    return NULL;
}

void aodbm_free_hash(aodbm_hash *ht) {
    free(ht->data);
    free(ht);
}
