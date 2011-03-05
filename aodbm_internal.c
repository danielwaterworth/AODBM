/*  
    aodbm - Append Only Database Manager
    Copyright (C) 2011 Daniel Waterworth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.f

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "string.h"

#include "pthread.h"

#include "aodbm_internal.h"
#include "aodbm_error.h"

#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) |\
    ntohl( ((uint32_t)(x >> 32)) ) )                                        
#define htonll(x) ntohll(x)

#ifdef AODBM_USE_MMAP
#include <unistd.h>
#include <sys/mman.h>
#endif

void print_hex(unsigned char c) {
    printf("\\x%.2x", c);
}

/* python style string printing */
void aodbm_print_data(aodbm_data *dat) {
    size_t i;
    for (i = 0; i < dat->sz; ++i) {
        char c = dat->dat[i];
        if (c == '\n') {
            printf("\\n");
        } else if (c >= 127 || c < 32) {
            print_hex(c);
        } else {
            putchar(c);
        }
    }
}

void annotate_data(const char *name, aodbm_data *val) {
    printf("%s: ", name);
    aodbm_print_data(val);
    putchar('\n');
}

void aodbm_print_rope(aodbm_rope *rope) {
    aodbm_data *dat = aodbm_rope_to_data(rope);
    aodbm_print_data(dat);
    aodbm_free_data(dat);
}

void annotate_rope(const char *name, aodbm_rope *val) {
    printf("%s: ", name);
    aodbm_print_rope(val);
    putchar('\n');
}

/*
  data format:
  v - v + 8 = prev version
  v + 8 = root node
  node - node + 1 = type (leaf of branch)
  node + 1 - node + 5 = node size
  branch:
  node + 5 ... = offset (key, offset)+
  leaf:
  node + 5 ... = (key, val)+
*/

aodbm_rope *make_block(aodbm_data *dat) {
    aodbm_rope *result = aodbm_data_to_rope_di(aodbm_data_from_32(dat->sz));
    aodbm_rope_append(result, dat);
    return result;
}

aodbm_rope *make_block_di(aodbm_data *dat) {
    return aodbm_data2_to_rope_di(aodbm_data_from_32(dat->sz), dat);
}

aodbm_rope *make_record(aodbm_data *key, aodbm_data *val) {
    return aodbm_rope_merge_di(make_block(key), make_block(val));
}

aodbm_rope *make_record_di(aodbm_data *key, aodbm_data *val) {
    return aodbm_rope_merge_di(make_block_di(key), make_block_di(val));
}

bool aodbm_read_bytes(aodbm *db, void *ptr, size_t sz) {
    if (fread(ptr, 1, sz, db->fd) != sz) {
        if (feof(db->fd)) {
            return false;
        }
        AODBM_OS_ERROR();
    }
    return true;
}

void aodbm_seek(aodbm *db, int64_t off, int startpoint) {
    if (fseeko(db->fd, off, startpoint) != 0) {
        AODBM_OS_ERROR();
    }
}

uint64_t aodbm_tell(aodbm *db) {
    return ftello(db->fd);
}

void aodbm_write_bytes(aodbm *db, void *ptr, size_t sz) {
    if (fwrite(ptr, 1, sz, db->fd) != sz) {
        AODBM_OS_ERROR();
    }
    db->file_size += sz;
}

void aodbm_truncate(aodbm *db, uint64_t sz) {
    if (ftruncate(fileno(db->fd), sz) != 0) {
        AODBM_OS_ERROR();
    }
    db->file_size = sz;
}

void aodbm_write_data_block(aodbm *db, aodbm_data *data) {
    pthread_mutex_lock(&db->rw);
    aodbm_write_bytes(db, "d", 1);
    /* ensure size fits in 32bits */
    uint32_t sz = htonl(data->sz);
    aodbm_write_bytes(db, &sz, 4);
    aodbm_write_bytes(db, data->dat, data->sz);
    fflush(db->fd);
    pthread_mutex_unlock(&db->rw);
}

void aodbm_write_version(aodbm *db, uint64_t ver) {
    pthread_mutex_lock(&db->rw);
    aodbm_write_bytes(db, "v", 1);
    uint64_t off = htonll(ver);
    aodbm_write_bytes(db, &off, 8);
    fflush(db->fd);
    pthread_mutex_unlock(&db->rw);
}

void aodbm_read(aodbm *db, uint64_t off, size_t sz, void *ptr) {
    #ifdef AODBM_USE_MMAP
    aodbm_rwlock_rdlock(&db->mmap_mut);
    
    if (db->mapping_size < off + (uint64_t)sz) {
        long page_size = sysconf(_SC_PAGE_SIZE);
        size_t new_size = db->file_size - (db->file_size % page_size);
        
        if (new_size < off + (uint64_t)sz) {
            aodbm_rwlock_unlock(&db->mmap_mut);
            
            pthread_mutex_lock(&db->rw);
            aodbm_seek(db, off, SEEK_SET);
            aodbm_read_bytes(db, ptr, sz);
            pthread_mutex_unlock(&db->rw);
            return;
        }
        
        aodbm_rwlock_unlock(&db->mmap_mut);
        aodbm_rwlock_wrlock(&db->mmap_mut);
        if (db->mapping == NULL) {
            db->mapping = mmap(NULL,
                               new_size,
                               PROT_READ,
                               MAP_SHARED,
                               fileno(db->fd),
                               0);
            if (db->mapping == MAP_FAILED) {
                AODBM_OS_ERROR();
            }
            db->mapping_size = new_size;
        } else {
            if (db->mapping_size < new_size) {
                db->mapping = mremap((void *)db->mapping,
                                     db->mapping_size,
                                     new_size,
                                     MREMAP_MAYMOVE);
                if (db->mapping == MAP_FAILED) {
                    AODBM_OS_ERROR();
                }
                db->mapping_size = new_size;
            }
        }
        aodbm_rwlock_unlock(&db->mmap_mut);
        aodbm_rwlock_rdlock(&db->mmap_mut);
    }
    
    memcpy(ptr, (void *)db->mapping + (size_t)off, sz);
    aodbm_rwlock_unlock(&db->mmap_mut);
    #else
    pthread_mutex_lock(&db->rw);
    aodbm_seek(db, off, SEEK_SET);
    aodbm_read_bytes(db, ptr, sz);
    pthread_mutex_unlock(&db->rw);
    #endif
}

uint32_t aodbm_read32(aodbm *db, uint64_t off) {
    uint32_t sz;
    aodbm_read(db, off, 4, &sz);
    return ntohl(sz);
}

uint64_t aodbm_read64(aodbm *db, uint64_t off) {
    uint64_t sz;
    aodbm_read(db, off, 8, &sz);
    return ntohll(sz);
}

aodbm_data *aodbm_read_data(aodbm *db, uint64_t off) {
    uint32_t sz = aodbm_read32(db, off);
    aodbm_data *out = malloc(sizeof(aodbm_data));
    out->sz = sz;
    out->dat = malloc(sz);
    aodbm_read(db, off+4, sz, out->dat);
    return out;
}

uint64_t aodbm_search_recursive(aodbm *db, uint64_t node, aodbm_data *key) {
    char type;
    aodbm_read(db, node, 1, &type);
    if (type == 'l') {
        return node;
    }
    if (type == 'b') {
        uint32_t sz = aodbm_read32(db, node + 1);
        uint64_t pos = node + 5;
        /* node begins with an offset */
        uint64_t off = aodbm_read64(db, pos);
        pos += 8;
        uint32_t i;
        for (i = 0; i < sz; ++i) {
            aodbm_data *dat = aodbm_read_data(db, pos);
            pos += dat->sz + 4;
            bool lt = aodbm_data_lt(key, dat);
            aodbm_free_data(dat);
            if (lt) {
                return aodbm_search_recursive(db, off, key);
            }
            off = aodbm_read64(db, pos);
            pos += 8;
        }
        return aodbm_search_recursive(db, off, key);
    } else {
        AODBM_CUSTOM_ERROR("unknown node type");
    }
}

/* returns the offset of the leaf node that the key belongs in */
uint64_t aodbm_search(aodbm *db, aodbm_version version, aodbm_data *key) {
    if (version == 0) {
        AODBM_CUSTOM_ERROR("error, given the 0 version for a search");
    }
    return aodbm_search_recursive(db, version + 8, key);
}

struct aodbm_stack {
    aodbm_stack *up;
    void *dat;
};

void aodbm_stack_push(aodbm_stack **stack, void *data) {
    aodbm_stack *new = malloc(sizeof(aodbm_stack));
    new->dat = data;
    new->up = *stack;
    *stack = new;
}

void *aodbm_stack_pop(aodbm_stack **stack) {
    if (*stack == NULL) {
        return NULL;
    }
    aodbm_stack *next = (*stack)->up;
    void *out = (*stack)->dat;
    free(*stack);
    *stack = next;
    return out;
}

void aodbm_search_path_recursive(aodbm *db,
                                 uint64_t node,
                                 aodbm_data *node_key,
                                 aodbm_data *key,
                                 aodbm_stack **path) {
    assert (aodbm_data_le(node_key, key));
    aodbm_path_node *path_node = malloc(sizeof(aodbm_path_node));
    path_node->key = node_key;
    path_node->node = node;
    aodbm_stack_push(path, (void *)path_node);
    
    char type;
    aodbm_read(db, node, 1, &type);
    if (type == 'l') {
        return;
    }
    if (type == 'b') {
        aodbm_data *prev_key = aodbm_data_dup(node_key);
        uint32_t sz = aodbm_read32(db, node + 1);
        uint64_t pos = node + 5;
        /* node begins with an offset */
        uint64_t off = aodbm_read64(db, pos);
        pos += 8;
        uint32_t i;
        aodbm_data *dat;
        for (i = 0; i < sz; ++i) {
            dat = aodbm_read_data(db, pos);
            pos += dat->sz + 4;
            if (aodbm_data_lt(key, dat)) {
                aodbm_free_data(dat);
                aodbm_search_path_recursive(db, off, prev_key, key, path);
                return;
            }
            aodbm_free_data(prev_key);
            prev_key = dat;
            off = aodbm_read64(db, pos);
            pos += 8;
        }
        aodbm_search_path_recursive(db, off, prev_key, key, path);
        return;
    } else {
        AODBM_CUSTOM_ERROR("unknown node type");
    }
}

aodbm_stack *aodbm_search_path(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        AODBM_CUSTOM_ERROR("error, given the 0 version for a search");
    }
    aodbm_stack *path = NULL;
    aodbm_search_path_recursive(db, ver + 8, aodbm_data_empty(), key, &path);
    return path;
}
