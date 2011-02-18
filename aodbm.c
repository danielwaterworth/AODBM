#define MAX_NODE_SIZE 4 /* IMPORTANT: the number must be even */

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) |\
    ntohl( ((uint32_t)(x >> 32)) ) )                                        
#define htonll(x) ntohll(x)

#include "aodbm.h"
#include "aodbm_internal.h"

struct aodbm {
    FILE *fd;
    pthread_mutex_t rw;
    volatile uint64_t cur;
    pthread_mutex_t version;
};

aodbm *aodbm_open(const char *filename) {
    aodbm *ptr = malloc(sizeof(aodbm));
    ptr->fd = fopen(filename, "a+b");
    
    pthread_mutexattr_t rec;
    pthread_mutexattr_init(&rec);
    pthread_mutexattr_settype(&rec, PTHREAD_MUTEX_RECURSIVE);
    
    pthread_mutex_init(&ptr->rw, &rec);
    pthread_mutex_init(&ptr->version, NULL);
    
    pthread_mutexattr_destroy(&rec);
    
    fseek(ptr->fd, 0, SEEK_SET);
    ptr->cur = 0;
    while(1) {
        char type;
        size_t n = fread(&type, 1, 1, ptr->fd);
        if (n != 1) {
            break;
        }
        if (type == 'v') {
            /* update version */
            uint64_t ver;
            if (fread(&ver, 1, 8, ptr->fd) != 8) {
                printf("error, unexpected EOF whilst reading a version\n");
                exit(1);
            }
            ptr->cur = ntohll(ver);
        } else if (type == 'd') {
            /* traverse data */
            uint32_t sz;
            if (fread(&sz, 1, 4, ptr->fd) != 4) {
                printf("error, unexpected EOF whilst reading a data block\n");
                exit(1);
            }
            sz = ntohl(sz);
            if (fseek(ptr->fd, sz, SEEK_CUR) != 0) {
                printf("error, cannot seek\n");
                exit(1);
            }
        } else {
            printf("error, unknown block type\n");
            exit(1);
        }
    }
    
    return ptr;
}

void aodbm_close(aodbm *db) {
    fclose(db->fd);
    pthread_mutex_destroy(&db->rw);
    pthread_mutex_destroy(&db->version);
    free(db);
}

uint64_t aodbm_file_size(aodbm *db) {
    uint64_t pos;
    pthread_mutex_lock(&db->rw);
    fseek(db->fd, 0, SEEK_END);
    pos = ftell(db->fd);
    pthread_mutex_unlock(&db->rw);
    return pos;
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

uint64_t aodbm_current(aodbm *db) {
    uint64_t result;
    pthread_mutex_lock(&db->version);
    result = db->cur;
    pthread_mutex_unlock(&db->version);
    return result;
}

bool aodbm_commit(aodbm *db, uint64_t version) {
    bool result;
    pthread_mutex_lock(&db->version);
    result = aodbm_is_based_on(db, version, db->cur);
    if (result) {
        /* write the new head */
        aodbm_write_version(db, version);
        db->cur = version;
    }
    pthread_mutex_unlock(&db->version);
    return result;
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

typedef struct {
    aodbm_rope *node;
    aodbm_data *key;
    uint64_t end_pos;
    uint32_t sz;
    bool inserted;
} range_result;

range_result add_header(range_result result) {
    aodbm_rope *header = aodbm_data2_to_rope_di(aodbm_data_from_str("l"),
                                                aodbm_data_from_32(result.sz));
    result.node = aodbm_rope_merge_di(header, result.node);
    return result;
}

range_result duplicate_range(aodbm *db,
                             uint64_t start_pos,
                             uint32_t num) {
    aodbm_rope *data = aodbm_rope_empty();
    aodbm_data *key1;
    uint64_t pos = start_pos;
    uint32_t i;
    for (i = 0; i < num; ++i) {
        aodbm_data *d_key = aodbm_read_data(db, pos);
        pos += d_key->sz + 4;
        aodbm_data *d_val = aodbm_read_data(db, pos);
        pos += d_val->sz + 4;
        
        if (i == 0) {
            key1 = aodbm_data_dup(d_key);
        }
        data = aodbm_rope_merge_di(data, make_record_di(d_key, d_val));
    }
    
    range_result result;
    result.node = data;
    result.key = key1;
    result.end_pos = pos;
    result.sz = num;
    result.inserted = false;
    return result;
}

range_result insert_into_range(aodbm *db,
                               aodbm_data *key,
                               aodbm_data *val,
                               uint64_t start_pos,
                               uint32_t num,
                               bool insert_end) {
    bool inserted;
    aodbm_rope *data = aodbm_rope_empty();
    aodbm_data *key1 = NULL;
    aodbm_rope *i_rec = make_record(key, val);
    uint64_t pos = start_pos;
    uint32_t i;
    for (i = 0; i < num; ++i) {
        aodbm_data *d_key = aodbm_read_data(db, pos);
        pos += d_key->sz + 4;
        aodbm_data *d_val = aodbm_read_data(db, pos);
        pos += d_val->sz + 4;
        
        int cmp = aodbm_data_cmp(key, d_key);
        if (cmp == -1) {
            inserted = true;
        }
        if (cmp == 0) {
            inserted = false;
        }
        if (cmp != 1) {
            data = aodbm_rope_merge_di(data, i_rec);
            if (i == 0) {
                key1 = aodbm_data_dup(key);
            }
        }
        if (cmp != 0) {
            if (key1 == NULL && i == 0) {
                key1 = aodbm_data_dup(d_key);
            }
            data = aodbm_rope_merge_di(data, make_record_di(d_key, d_val));
        }
        if (cmp != 1) {
            break;
        }
    }
    range_result result;
    result.key = key1;
    result.inserted = true;
    if (i < num) {
        /* the loop was broken */
        range_result dup = duplicate_range(db, pos, num - i);
        aodbm_free_data(dup.key);
        
        result.node = aodbm_rope_merge_di(data, dup.node);
        result.end_pos = dup.end_pos;
        result.sz = num + (inserted?1:0);
    } else {
        if (insert_end) {
            result.node = aodbm_rope_merge_di(data, i_rec);
            result.sz = num + 1;
        } else {
            result.node = data;
            result.inserted = false;
            result.sz = num;
        }
        result.end_pos = pos;
    }
    return result;
}

typedef struct {
    uint64_t a, b;
    aodbm_data *append;
} set_result;

typedef struct {
    aodbm_rope *a_node;
    aodbm_data *a_key;
    aodbm_rope *b_node;
    aodbm_data *b_key;
} leaf_result;

/* pos is positioned just after the type byte */
leaf_result insert_into_leaf(aodbm *db,
                             aodbm_data *key,
                             aodbm_data *val,
                             uint64_t pos) {
    uint32_t sz = aodbm_read32(db, pos);
    pos += 4;
    if (sz == MAX_NODE_SIZE) {
        range_result a_range =
            add_header(
                insert_into_range(db, key, val, pos, MAX_NODE_SIZE/2, false));
        range_result b_range;
        if (a_range.inserted) {
            b_range =
                add_header(
                    duplicate_range(db, a_range.end_pos, MAX_NODE_SIZE/2));
        } else {
            b_range = add_header(insert_into_range(db,
                                                   key,
                                                   val,
                                                   a_range.end_pos,
                                                   MAX_NODE_SIZE/2,
                                                   true));
        }
        leaf_result result;
        result.a_node = a_range.node;
        result.a_key = a_range.key;
        result.b_node = b_range.node;
        result.b_key = b_range.key;
        return result;
    } else if (sz < MAX_NODE_SIZE) {
        range_result range =
            add_header(insert_into_range(db, key, val, pos, sz, true));
        leaf_result result;
        result.a_node = range.node;
        result.a_key = range.key;
        result.b_node = NULL;
        result.b_key = NULL;
        return result;
    } else {
        printf("found a node with a size beyond MAX_NODE_SIZE\n");
        exit(1);
    }
}

/*
  Given a node, return a new version of the node
  the node must be ondisk
*/
set_result aodbm_set_recursive(aodbm *db,
                               uint64_t node,
                               aodbm_data *key,
                               aodbm_data *val,
                               aodbm_data *append,
                               uint64_t append_pos) {
    char type;
    aodbm_read(db, node, 1, &type);
    if (type != 'l' && type != 'b') {
        printf("unknown node type\n");
        exit(1);
    }
    uint32_t sz = aodbm_read32(db, node + 1);
    uint64_t pos = node + 5;
    if (type == 'b') {
        exit(1);
    } else {
        exit(1);
    }
}

aodbm_version aodbm_set(aodbm *db,
                        aodbm_version ver,
                        aodbm_data *key,
                        aodbm_data *val) {
    pthread_mutex_lock(&db->rw);
    /* find the position of the appendment (filesize + data block header) */
    uint64_t append_pos = aodbm_file_size(db) + 5;
    aodbm_version result;
    aodbm_data *root = aodbm_data_from_64(ver);
    aodbm_data *append;
    
    if (ver == 0) {
        aodbm_rope *node = aodbm_data2_to_rope_di(aodbm_data_from_str("l"),
                                                  aodbm_data_from_32(1));
        node = aodbm_rope_merge_di(node, make_record(key, val));
        aodbm_rope_prepend_di(root, node);
        append = aodbm_rope_to_data_di(node);
        result = append_pos;
    } else {
        char type;
        aodbm_read(db, ver + 8, 1, &type);
        if (type == 'l') {
            leaf_result leaf = insert_into_leaf(db, key, val, ver + 9);
            aodbm_free_data(leaf.a_key);
            if (leaf.b_node == NULL) {
                aodbm_rope *data = leaf.a_node;
                aodbm_rope_prepend_di(root, data);
                append = aodbm_rope_to_data_di(data);
                result = append_pos;
            } else {
                size_t a_sz = aodbm_rope_size(leaf.a_node);
                size_t b_sz = aodbm_rope_size(leaf.b_node);
                
                aodbm_rope *data = aodbm_rope_merge_di(leaf.a_node, leaf.b_node);
                size_t data_sz = a_sz + b_sz;
                /* construct a new branch node */
                aodbm_rope *br = aodbm_data2_to_rope_di(aodbm_data_from_str("b"),
                                                        aodbm_data_from_32(1));
                /* a position */
                aodbm_rope_append_di(br, aodbm_data_from_64(append_pos));
                /* b_key */
                br = aodbm_rope_merge_di(br, make_block_di(leaf.b_key));
                /* b position */
                aodbm_rope_append_di(br, aodbm_data_from_64(append_pos + a_sz));
                aodbm_rope_prepend_di(root, br);
                
                data = aodbm_rope_merge_di(data, br);
                
                append = aodbm_rope_to_data_di(data);
                result = append_pos + data_sz;
            }
        } else if (type = 'b') {
            exit(1);
        } else {
            printf("unknown node type\n");
            exit(1);
        }
    }
    
    aodbm_write_data_block(db, append);
    aodbm_free_data(append);
    
    pthread_mutex_lock(&db->rw);
    
    return result;
}

bool aodbm_has_recursive(aodbm *db, uint64_t node, aodbm_data *key) {
    char type;
    aodbm_read(db, node, 1, &type);
    if (type != 'l' && type != 'b') {
        printf("unknown node type\n");
        exit(1);
    }
    uint32_t sz = aodbm_read32(db, node + 1);
    uint32_t i;
    uint64_t pos = node + 5;
    if (type == 'b') {
        /* node begins with an offset */
        uint64_t off = aodbm_read64(db, pos);
        pos += 8;
        for (i = 0; i < sz; ++i) {
            aodbm_data *dat = aodbm_read_data(db, pos);
            pos += dat->sz + 4;
            bool lt = aodbm_data_lt(key, dat);
            aodbm_free_data(dat);
            if (lt) {
                return aodbm_has_recursive(db, off, key);
            }
            off = aodbm_read64(db, pos);
            pos += 8;
        }
        return aodbm_has_recursive(db, off, key);
    } else {
        for (i = 0; i < sz; ++i) {
            aodbm_data *dat = aodbm_read_data(db, pos);
            pos += dat->sz + 4;
            bool eq = aodbm_data_eq(dat, key);
            aodbm_free_data(dat);
            if (eq) {
                return true;
            }
            aodbm_data *val = aodbm_read_data(db, pos);
            pos += val->sz + 4;
            aodbm_free_data(val);
        }
        return false;
    }
}

bool aodbm_has(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        return false;
    }
    return aodbm_has_recursive(db, ver + 8, key);
}

aodbm_data *aodbm_get_recursive(aodbm *db, uint64_t node, aodbm_data *key) {
    char type;
    aodbm_read(db, node, 1, &type);
    if (type != 'l' && type != 'b') {
        printf("unknown node type\n");
        exit(1);
    }
    uint32_t sz = aodbm_read32(db, node + 1);
    uint32_t i;
    uint64_t pos = node + 5;
    if (type == 'b') {
        /* node begins with an offset */
        uint64_t off = aodbm_read64(db, pos);
        pos += 8;
        for (i = 0; i < sz; ++i) {
            aodbm_data *dat = aodbm_read_data(db, pos);
            pos += dat->sz + 4;
            if (aodbm_data_lt(key, dat)) {
                aodbm_free_data(dat);
                return aodbm_get_recursive(db, off, key);
            }
            aodbm_free_data(dat);
            off = aodbm_read64(db, pos);
            pos += 8;
        }
        return aodbm_get_recursive(db, off, key);
    } else {
        for (i = 0; i < sz; ++i) {
            aodbm_data *dat = aodbm_read_data(db, pos);
            pos += dat->sz + 4;
            aodbm_data *val = aodbm_read_data(db, pos);
            pos += val->sz + 4;
            if (aodbm_data_eq(dat, key)) {
                return val;
            }
            aodbm_free_data(dat);
            aodbm_free_data(val);
        }
        return NULL;
    }
}

aodbm_data *aodbm_get(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        return NULL;
    }
    return aodbm_get_recursive(db, ver + 8, key);
}

aodbm_version aodbm_del(aodbm *db, aodbm_version ver, aodbm_data *key) {
    exit(1);
}

bool aodbm_is_based_on(aodbm *db, aodbm_version a, aodbm_version b) {
    /* is a based on b? */
    if (b == 0) {
        /* everything is based on version 0 */
        return true;
    }
    if (a == 0) {
        /* version 0 is based on nothing */
        return false;
    }
    if (a < b) {
        /* a can't be based on a version that is further on in the file */
        return false;
    }
    if (a == b) {
        return true;
    }
    return aodbm_is_based_on(db, aodbm_read64(db, a), b);
}
