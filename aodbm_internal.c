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

#include "aodbm_internal.h"

#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) |\
    ntohl( ((uint32_t)(x >> 32)) ) )                                        
#define htonll(x) ntohll(x)

#ifdef AODBM_USE_MMAP
#include <unistd.h>
#include <fcntl.h>

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
        perror("aodbm");
        exit(1);
    }
    return true;
}

void aodbm_seek(aodbm *db, int64_t off, int startpoint) {
    if (fseeko(db->fd, off, startpoint) != 0) {
        perror("aodbm");
        exit(1);
    }
}

uint64_t aodbm_tell(aodbm *db) {
    return ftello(db->fd);
}

void aodbm_write_bytes(aodbm *db, void *ptr, size_t sz) {
    db->file_size += sz;
    if (fwrite(ptr, 1, sz, db->fd) != sz) {
        perror("aodbm");
        exit(1);
    }
}

void aodbm_truncate(aodbm *db, uint64_t sz) {
    if (ftruncate(fileno(db->fd), sz) != 0) {
        perror("aodbm");
        exit(1);
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
    long page_size = sysconf(_SC_PAGE_SIZE);
    
    uint64_t start, end;
    start = off - (off % page_size);
    end = off + sz;
    
    if (end % page_size != 0) {
        end = end - (end % page_size) + page_size;
    }
    
    size_t map_sz = end - start;
    
    if (end < aodbm_file_size(db)) {
        void *mapping =
            mmap(NULL, map_sz, PROT_READ, MAP_SHARED, fileno(db->fd), start);
        if (mapping == MAP_FAILED) {
            perror("aodbm");
            exit(1);
        }
        memcpy(ptr, mapping + (off - start), sz);
        munmap(mapping, map_sz);
    } else {
        pthread_mutex_lock(&db->rw);
        aodbm_seek(db, off, SEEK_SET);
        aodbm_read_bytes(db, ptr, sz);
        pthread_mutex_unlock(&db->rw);
    }
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
        printf("unknown node type\n");
        exit(1);
    }
}

/* returns the offset of the leaf node that the key belongs in */
uint64_t aodbm_search(aodbm *db, aodbm_version version, aodbm_data *key) {
    if (version == 0) {
        printf("error, given the 0 version for a search\n");
        exit(1);
    }
    return aodbm_search_recursive(db, version + 8, key);
}

struct aodbm_path {
    aodbm_path *up;
    aodbm_path_node node;
};

void aodbm_path_print(aodbm_path *path) {
    aodbm_path *it;
    printf("[");
    for (it = path; it->up != NULL; it = it->up) {
        printf("%llu, ", (long long unsigned int)it->node.node);
    }
    printf("%llu]", (long long unsigned int)it->node.node);
}

void aodbm_path_push(aodbm_path **path, aodbm_path_node node) {
    aodbm_path *new = malloc(sizeof(aodbm_path));
    new->node = node;
    if (*path == NULL) {
        new->up = NULL;
    } else {
        new->up = *path;
    }
    *path = new;
}

aodbm_path_node aodbm_path_pop(aodbm_path **path) {
    if (*path == NULL) {
        printf("cannot pop an empty aodbm_path\n");
        exit(1);
    } else {
        aodbm_path_node result = (*path)->node;
        aodbm_path *fr = *path;
        *path = (*path)->up;
        free(fr);
        return result;
    }
}

void aodbm_search_path_recursive(aodbm *db,
                                 uint64_t node,
                                 aodbm_data *node_key,
                                 aodbm_data *key,
                                 aodbm_path **path) {
    assert (aodbm_data_le(node_key, key));
    aodbm_path_node path_node = {node_key, node};
    aodbm_path_push(path, path_node);
    
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
        printf("unknown node type\n");
        exit(1);
    }
}

aodbm_path *aodbm_search_path(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        printf("error, given the 0 version for a search\n");
        exit(1);
    }
    aodbm_path *path = NULL;
    aodbm_search_path_recursive(db, ver + 8, aodbm_data_empty(), key, &path);
    return path;
}
