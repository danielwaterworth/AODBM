#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

#include "aodbm_internal.h"

#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) |\
    ntohl( ((uint32_t)(x >> 32)) ) )                                        
#define htonll(x) ntohll(x)

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

void aodbm_write_data_block(aodbm *db, aodbm_data *data) {
    pthread_mutex_lock(&db->rw);
    fwrite("d", 1, 1, db->fd);
    /* ensure size fits in 32bits */
    uint32_t sz = htonl(data->sz);
    fwrite(&sz, 1, 4, db->fd);
    fwrite(data->dat, 1, data->sz, db->fd);
    pthread_mutex_unlock(&db->rw);
}

void aodbm_write_version(aodbm *db, uint64_t ver) {
    pthread_mutex_lock(&db->rw);
    fwrite("v", 1, 1, db->fd);
    uint64_t off = htonll(ver);
    fwrite(&off, 1, 8, db->fd);
    pthread_mutex_unlock(&db->rw);
}

void aodbm_read(aodbm *db, uint64_t off, size_t sz, void *ptr) {
    /* TODO: modify to use mmap */
    pthread_mutex_lock(&db->rw);
    fseek(db->fd, off, SEEK_SET);
    if (fread(ptr, 1, sz, db->fd) != sz) {
        printf("unexpected EOF\n");
        assert(0);
        exit(1);
    }
    pthread_mutex_unlock(&db->rw);
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
        printf("%llu, ", it->node.node);
    }
    printf("%llu]", it->node.node);
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
