#define MAX_NODE_SIZE 10

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"

#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) |\
    ntohl( ((uint32_t)(x >> 32)) ) )                                        
#define htonll(x) ntohll(x)

#include "aodbm.h"

struct aodbm {
    FILE *fd;
    pthread_mutex_t rw;
    volatile uint64_t cur;
    pthread_mutex_t version;
};

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

void aodbm_rope_prepend_di(aodbm_rope *rope, aodbm_data *dat) {
    aodbm_rope_node *node = aodbm_data_to_rope_node_di(dat);
    node->next = rope->first;
    rope->first = node;
}

void aodbm_rope_append(aodbm_rope *rope, aodbm_data *dat) {
    aodbm_rope_append(rope, aodbm_data_dup(dat));
}

void aodbm_rope_prepend(aodbm_rope *rope, aodbm_data *dat) {
    aodbm_rope_prepend(rope, aodbm_data_dup(dat));
}

aodbm_rope *aodbm_rope_merge_di(aodbm_rope *a, aodbm_rope *b) {
    a->last->next = b->first;
    a->last = b->last;
    free(b);
    return a;
}

void print_hex(c) {
    printf("\\x%.2x", c);
}

/* python style string printing */
void aodbm_print(aodbm_data *dat) {
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

void annotate(const char *name, aodbm_data *val) {
    printf("%s: ", name);
    aodbm_print(val);
    putchar('\n');
}

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
    /* modify to use mmap */
    pthread_mutex_lock(&db->rw);
    fseek(db->fd, off, SEEK_SET);
    if (fread(ptr, 1, sz, db->fd) != sz) {
        printf("unexpected EOF\n");
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

aodbm_data *make_block(aodbm_data *dat) {
    aodbm_data *sz = aodbm_data_from_32(dat->sz);
    aodbm_data *rec = aodbm_cat_data(sz, dat);
    aodbm_free_data(sz);
    return rec;
}

aodbm_data *make_block_di(aodbm_data *dat) {
    return aodbm_cat_data_di(aodbm_data_from_32(dat->sz), dat);
}

aodbm_data *make_record(aodbm_data *key, aodbm_data *val) {
    return aodbm_cat_data_di(make_block(key), make_block(val));
}

aodbm_data *make_record_di(aodbm_data *key, aodbm_data *val) {
    return aodbm_cat_data_di(make_block_di(key), make_block_di(val));
}

typedef struct {
    aodbm_data *node;
    aodbm_data *key;
    uint64_t end_pos;
    uint32_t sz;
} range_result;

range_result add_header(range_result result) {
    aodbm_data *header = aodbm_cat_data_di(aodbm_data_from_str("l"),
                                           aodbm_data_from_32(result.sz));
    result.node = aodbm_cat_data_di(header, result.node);
    return result;
}

range_result duplicate_range(aodbm *db,
                             uint64_t start_pos,
                             uint32_t num) {
    aodbm_data *data = aodbm_data_empty();
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
        data = aodbm_cat_data_di(data, make_record_di(d_key, d_val));
    }
    
    range_result result;
    result.node = data;
    result.key = key1;
    result.end_pos = pos;
    result.sz = num;
    return result;
}

range_result insert_into_range(aodbm *db,
                               aodbm_data *key,
                               aodbm_data *val,
                               uint64_t start_pos,
                               uint32_t num) {
    aodbm_data *data = aodbm_data_empty();
    aodbm_data *key1 = NULL;
    aodbm_data *i_rec = make_record(key, val);
    uint64_t pos = start_pos;
    uint32_t i;
    bool inserted;
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
            data = aodbm_cat_data_di(data, i_rec);
            if (i == 0) {
                key1 = aodbm_data_dup(key);
            }
        }
        if (cmp != 0) {
            if (key1 == NULL && i == 0) {
                key1 = aodbm_data_dup(d_key);
            }
            data = aodbm_cat_data_di(data, make_record_di(d_key, d_val));
        }
        if (cmp != 1) {
            break;
        }
    }
    range_result result;
    result.key = key1;
    if (i < num) {
        /* the loop was broken */
        range_result dup = duplicate_range(db, pos, num - i);
        aodbm_free_data(dup.key);
        
        result.node = aodbm_cat_data_di(data, dup.node);
        result.end_pos = dup.end_pos;
        result.sz = num + (inserted?1:0);
    } else {
        result.node = aodbm_cat_data_di(data, i_rec);
        result.end_pos = pos;
        result.sz = num + 1;
    }
    return result;
}

typedef struct {
    uint64_t a, b;
    aodbm_data *append;
} set_result;

typedef struct {
    aodbm_data *a_node;
    aodbm_data *b_node;
    aodbm_data *b_key;
} leaf_result;

/* pos is positioned just after the type byte */
leaf_result insert_into_leaf(aodbm *db,
                             aodbm_data *key,
                             aodbm_data *val,
                             uint64_t pos) {
    uint32_t sz = aodbm_read32(db, pos);
    if (sz == MAX_NODE_SIZE) {
        
    } else if (sz < MAX_NODE_SIZE) {
        range_result range = add_header(insert_into_range(db, key, val, pos, sz));
        aodbm_free_data(range.key);
    
        
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
        if (sz == MAX_NODE_SIZE) {
            /*split_leaf_result res = aodbm_split_leaf(db, key, val, pos);
            if (res.b_node != NULL) {
                
            } else { 
            
            }*/
        } else if (sz < MAX_NODE_SIZE) {
            /*aodbm_data *res = aodbm_simple_insert_leaf(db, key, val, pos, sz);
            set_result result;
            result.a = append_pos;
            result.b = 0;
            result.append = res;
            return result;*/
        } else {
            printf("found a node with greater than max node size\n");
            exit(1);
        }
    }
}

aodbm_version aodbm_set(aodbm *db,
                        aodbm_version ver,
                        aodbm_data *key,
                        aodbm_data *val) {
    pthread_mutex_lock(&db->rw);
    /* find the position of the appendment (filesize + data block header) */
    uint64_t pos = aodbm_file_size(db) + 5;
    aodbm_version result;
    aodbm_data *root = aodbm_data_from_64(ver);
    aodbm_data *append;
    
    if (ver == 0) {
        aodbm_data *node = aodbm_cat_data_di(aodbm_data_from_str("l"),
                                             aodbm_data_from_32(1));
        node = aodbm_cat_data_di(node, make_record(key, val));
        append = aodbm_cat_data_di(root, node);
        result = pos;
    } else {
        char type;
        aodbm_read(db, ver + 8, 1, &type);
        uint32_t sz = aodbm_read32(db, ver + 9);
        if (type == 'l') {
            if (sz == MAX_NODE_SIZE) {
                #if 0
                split_leaf_result res =
                    aodbm_split_leaf(db, key, val, ver + 13);
                if (res.b_node == NULL) {
                    append = aodbm_cat_data_di(root, res.append);
                    result = pos;
                } else {
                    exit(1);
                    /* make a new branch node */
                    /* type */
                    aodbm_data *node = aodbm_data_from_str("b");
                    /* sz */
                    node = aodbm_cat_data_di(node, aodbm_data_from_32(2));
                    /* a_reference */
                    node = aodbm_cat_data_di(node, aodbm_data_from_64(res.a));
                    /* TODO: insert key */
                    /* the key is the first entry in the second node */
                    
                    /* b_reference */
                    node = aodbm_cat_data_di(node, aodbm_data_from_64(res.b));
                    node = aodbm_cat_data_di(root, node);
                    result = pos + res.append->sz;
                    append = aodbm_cat_data_di(res.append, node);
                }
                #endif
            } else if (sz < MAX_NODE_SIZE) {
                #if 0
                append = aodbm_simple_insert_leaf(db,
                                                  key,
                                                  val,
                                                  ver + 13,
                                                  sz);
                append = aodbm_cat_data_di(root, append);
                result = pos;
                #endif
            } else {
                printf("found a node with greater than max node size\n");
                exit(1);
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

aodbm_data *aodbm_construct_data(const char *dat, size_t sz) {
    aodbm_data *ptr = malloc(sizeof(aodbm_data));
    ptr->sz = sz;
    ptr->dat = malloc(sz);
    memcpy(ptr->dat, dat, sz);
    return ptr;
}

aodbm_data *aodbm_cat_data(aodbm_data *a, aodbm_data *b) {
    aodbm_data *dat = malloc(sizeof(aodbm_data));
    dat->sz = a->sz + b->sz;
    dat->dat = malloc(dat->sz);
    memcpy(dat->dat, a->dat, a->sz);
    memcpy(dat->dat + a->sz, b->dat, b->sz);
    return dat;
}

aodbm_data *aodbm_cat_data_di(aodbm_data *a, aodbm_data *b) {
    aodbm_data *res = aodbm_cat_data(a, b);
    aodbm_free_data(a);
    aodbm_free_data(b);
    return res;
}

aodbm_data *aodbm_data_from_str(const char *dat) {
    return aodbm_construct_data(dat, strlen(dat));
}

aodbm_data *aodbm_data_from_32(uint32_t n) {
    n = htonl(n);
    return aodbm_construct_data((const char*)&n, 4);
}

aodbm_data *aodbm_data_from_64(uint64_t n) {
    n = htonll(n);
    return aodbm_construct_data((const char*)&n, 8);
}

aodbm_data *aodbm_data_empty() {
    aodbm_data *dat = malloc(sizeof(aodbm_data));
    dat->sz = 0;
    dat->dat = NULL;
    return dat;
}

bool aodbm_data_lt(aodbm_data *a, aodbm_data *b) {
    int p;
    int m = a->sz;
    if (b->sz < m) {
        m = b->sz;
    }
    for (p = 0; p < m; ++p) {
        if (a->dat[p] < b->dat[p])
            return true;
        if (a->dat[p] > b->dat[p])
            return false;
    }
    if (a->sz < b->sz) {
        return true;
    }
    return false;
}

bool aodbm_data_gt(aodbm_data *a, aodbm_data *b) {
    return aodbm_data_lt(b, a);
}

bool aodbm_data_lte(aodbm_data *a, aodbm_data *b) {
    return !aodbm_data_gt(a, b);
}

bool aodbm_data_gte(aodbm_data *a, aodbm_data *b) {
    return !aodbm_data_lt(a, b);
}

bool aodbm_data_eq(aodbm_data *a, aodbm_data *b) {
    if (a->sz != b->sz) {
        return false;
    }
    uint32_t i;
    for (i = 0; i < a->sz; ++i) {
        if (a->dat[i] != b->dat[i]) {
            return false;
        }
    }
    return true;
}

int aodbm_data_cmp(aodbm_data *a, aodbm_data *b) {
    if (aodbm_data_eq(a, b)) {
        return 0;
    }
    if (aodbm_data_lt(a, b)) {
        return -1;
    }
    return 1;
}

aodbm_data *aodbm_data_dup(aodbm_data *v) {
    return aodbm_construct_data(v->dat, v->sz);
}

void aodbm_free_data(aodbm_data *dat) {
    free(dat->dat);
    free(dat);
}
