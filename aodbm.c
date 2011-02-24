/* IMPORTANT: this number must be even and >= 4
   a higher number means bigger files, longer writes, faster reads (to a point)
*/
#define MAX_NODE_SIZE 4

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

#include <arpa/inet.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) |\
    ntohl( ((uint32_t)(x >> 32)) ) )                                        
#define htonll(x) ntohll(x)

#ifdef AODBM_USE_MMAP
#include <unistd.h>
#include <fcntl.h>
#endif

#include "aodbm.h"
#include "aodbm_internal.h"

aodbm *aodbm_open(const char *filename) {
    aodbm *ptr = malloc(sizeof(aodbm));
    #ifdef AODBM_USE_MMAP
    ptr->fd = open(filename, O_RDWR|O_APPEND|O_CREAT|O_FSYNC, 0644);
    if (ptr->fd < 0) {
        printf("couldn't open file\n");
        exit(1);
    }
    ptr->file_size = 0;
    #else
    ptr->fd = fopen(filename, "a+b");
    if (ptr->fd == NULL) {
        printf("couldn't open file\n");
        exit(1);
    }
    #endif
    
    pthread_mutexattr_t rec;
    pthread_mutexattr_init(&rec);
    pthread_mutexattr_settype(&rec, PTHREAD_MUTEX_RECURSIVE);
    
    pthread_mutex_init(&ptr->rw, &rec);
    pthread_mutex_init(&ptr->version, NULL);
    
    pthread_mutexattr_destroy(&rec);
    
    aodbm_seek(ptr, 0, SEEK_SET);
    ptr->cur = 0;
    while(1) {
        char type;
        if (!aodbm_read_bytes(ptr, &type, 1)) {
            break;
        }
        ptr->file_size += 1;
        if (type == 'v') {
            /* update version */
            uint64_t ver;
            if (!aodbm_read_bytes(ptr, &ver, 8)) {
                /* TODO: modify to truncate */
                printf("error, unexpected EOF whilst reading a version\n");
                exit(1);
            }
            ptr->cur = ntohll(ver);
            ptr->file_size += 8;
        } else if (type == 'd') {
            /* traverse data */
            uint32_t sz;
            if (!aodbm_read_bytes(ptr, &sz, 4)) {
                /* TODO: modify to truncate */
                printf("error, unexpected EOF whilst reading a data block\n");
                exit(1);
            }
            ptr->file_size += 4;
            sz = ntohl(sz);
            if (!aodbm_seek(ptr, sz, SEEK_CUR)) {
                /* TODO: check for EOF, truncate file */
                printf("error, cannot seek\n");
                exit(1);
            }
            ptr->file_size += sz;
        } else {
            printf("error, unknown block type\n");
            exit(1);
        }
    }
    
    return ptr;
}

void aodbm_close(aodbm *db) {
    #ifdef AODBM_USE_MMAP
    close(db->fd);
    #else
    fclose(db->fd);
    #endif
    pthread_mutex_destroy(&db->rw);
    pthread_mutex_destroy(&db->version);
    free(db);
}

uint64_t aodbm_file_size(aodbm *db) {
    #ifdef AODBM_USE_MMAP
    return db->file_size;
    #else
    uint64_t pos;
    pthread_mutex_lock(&db->rw);
    aodbm_seek(db, 0, SEEK_END);
    pos = aodbm_tell(db);
    pthread_mutex_unlock(&db->rw);
    return pos;
    #endif
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

bool aodbm_commit_init(aodbm *db, uint64_t version) {
    pthread_mutex_lock(&db->version);
    return aodbm_is_based_on(db, version, db->cur);
}

void aodbm_commit_finish(aodbm *db, uint64_t version) {
    aodbm_write_version(db, version);
    db->cur = version;
    pthread_mutex_unlock(&db->version);
}

void aodbm_commit_abort(aodbm *db) {
    pthread_mutex_unlock(&db->version);
}

aodbm_rope *aodbm_branch_di(uint64_t a, aodbm_data *key, uint64_t b) {
    aodbm_rope *br = aodbm_data2_to_rope_di(aodbm_data_from_str("b"),
                                            aodbm_data_from_32(1));
    aodbm_rope_append_di(br, aodbm_data_from_64(a));
    br = aodbm_rope_merge_di(br, make_block_di(key));
    aodbm_rope_append_di(br, aodbm_data_from_64(b));
    return br;
}

aodbm_rope *aodbm_leaf_node(aodbm_data *key, aodbm_data *val) {
    aodbm_rope *node = aodbm_data2_to_rope_di(aodbm_data_from_str("l"),
                                              aodbm_data_from_32(1));
    return aodbm_rope_merge_di(node, make_record(key, val));
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

range_result duplicate_leaf_range(aodbm *db,
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

range_result insert_into_leaf_range(aodbm *db,
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
        if (num - i != 1) {
            range_result dup = duplicate_leaf_range(db, pos, num - i - 1);
            aodbm_free_data(dup.key);
        
            result.node = aodbm_rope_merge_di(data, dup.node);
            result.end_pos = dup.end_pos;
        } else {
            result.node = data;
            result.end_pos = pos;
        }
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
    aodbm_rope *a_node;
    aodbm_data *a_key;
    aodbm_rope *b_node;
    aodbm_data *b_key;
} insert_result;

/* pos is positioned just after the type byte */
insert_result insert_into_leaf(aodbm *db,
                               aodbm_data *key,
                               aodbm_data *val,
                               uint64_t pos) {
    uint32_t sz = aodbm_read32(db, pos);
    pos += 4;
    if (sz == MAX_NODE_SIZE) {
        range_result a_range =
            add_header(
                insert_into_leaf_range(db, key, val, pos, MAX_NODE_SIZE/2, false));
        range_result b_range;
        if (a_range.inserted) {
            b_range =
                add_header(
                    duplicate_leaf_range(db, a_range.end_pos, MAX_NODE_SIZE/2));
        } else {
            b_range = add_header(insert_into_leaf_range(db,
                                                   key,
                                                   val,
                                                   a_range.end_pos,
                                                   MAX_NODE_SIZE/2,
                                                   true));
        }
        insert_result result;
        result.a_node = a_range.node;
        result.a_key = a_range.key;
        result.b_node = b_range.node;
        result.b_key = b_range.key;
        return result;
    } else if (sz == 0) {
        insert_result result;
        result.a_node = aodbm_data2_to_rope_di(aodbm_data_from_str("l"),
                                               aodbm_data_from_32(1));
        result.a_node = aodbm_rope_merge_di(result.a_node,
                                            make_record(key, val));
        result.a_key = aodbm_data_dup(key);
        result.b_node = NULL;
        result.b_key = NULL;
        return result;
    } else if (sz < MAX_NODE_SIZE) {
        range_result range =
            add_header(insert_into_leaf_range(db, key, val, pos, sz, true));
        insert_result result;
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

typedef struct {
    aodbm_rope *data;
    aodbm_data *key;
} remove_result;

remove_result remove_from_leaf(aodbm *db,
                               aodbm_data *key,
                               uint64_t pos) {
    remove_result result;
    result.key = NULL;
    aodbm_rope *data = aodbm_rope_empty();
    uint32_t sz = aodbm_read32(db, pos);
    pos += 4;
    
    uint32_t i;
    bool removed = false;
    for (i = 0; i < sz; ++i) {
        aodbm_data *r_key = aodbm_read_data(db, pos);
        pos += 4 + r_key->sz;
        aodbm_data *r_val = aodbm_read_data(db, pos);
        pos += 4 + r_val->sz;
        
        if (!removed) {
            if (!aodbm_data_eq(r_key, key)) {
                if (result.key == NULL) {
                    result.key = aodbm_data_dup(r_key);
                }
                data = aodbm_rope_merge_di(data, make_record_di(r_key, r_val));
            } else {
                removed = true;
                aodbm_free_data(r_key);
                aodbm_free_data(r_val);
            }
        } else {
            if (result.key == NULL) {
                result.key = aodbm_data_dup(r_key);
            }
            data = aodbm_rope_merge_di(data, make_record_di(r_key, r_val));
        }
    }
    
    aodbm_rope *header =
        aodbm_data2_to_rope_di(aodbm_data_from_str("l"),
                               aodbm_data_from_32(sz - (removed?1:0)));
    data = aodbm_rope_merge_di(header, data);
    
    result.data = data;
    return result;
}

typedef struct {
    aodbm_rope *data;
    aodbm_data *key;
    uint32_t sz;
} branch;

void branch_init(branch *br) {
    br->data = NULL;
    br->key = NULL;
    br->sz = 0;
}

void add_to_branch_di(branch *node, aodbm_data *key, uint64_t off) {
    aodbm_data *d_off = aodbm_data_from_64(off);
    if (node->key == NULL) {
        node->key = key;
        node->data = aodbm_data_to_rope_di(d_off);
    } else {
        aodbm_rope *rec = make_block_di(key);
        aodbm_rope_append_di(rec, d_off);
        node->data = aodbm_rope_merge_di(node->data, rec);
    }
    node->sz += 1;
}

void add_to_branch(branch *node, aodbm_data *key, uint64_t off) {
    add_to_branch_di(node, aodbm_data_dup(key), off);
}

void add_to_branches_di(branch *a, branch *b, aodbm_data *key, uint64_t off) {
    if (a->sz + 1 < MAX_NODE_SIZE/2) {
        add_to_branch_di(a, key, off);
    } else {
        add_to_branch_di(b, key, off);
    }
}

void add_to_branches(branch *a, branch *b, aodbm_data *key, uint64_t off) {
    add_to_branches_di(a, b, aodbm_data_dup(key), off);
}

void merge_branches(branch *a, branch *b) {
    if (b->sz) {
        a->sz += b->sz;
        a->data = aodbm_rope_merge_di(a->data, make_block_di(b->key));
        a->data = aodbm_rope_merge_di(a->data, b->data);
        b->sz = 0;
    }
}

aodbm_rope *branch_to_rope(branch *br) {
    aodbm_rope *header = aodbm_data2_to_rope(aodbm_data_from_str("b"),
                                             aodbm_data_from_32(br->sz - 1));
    return aodbm_rope_merge_di(header, br->data);
}

insert_result insert_into_branch(aodbm *db,
                                 uint64_t node,
                                 aodbm_data *node_key,
                                 uint64_t node_a,
                                 aodbm_data *a_key,
                                 uint64_t node_b,
                                 aodbm_data *b_key,
                                 uint64_t rm) {
    branch a, b;
    branch_init(&a);
    branch_init(&b);
    
    uint64_t pos = node + 1;
    uint32_t sz = aodbm_read32(db, pos);
    pos += 4;
    
    uint64_t off = aodbm_read64(db, pos);
    pos += 8;
    
    uint32_t i;
    bool a_placed = false;
    bool b_placed = b_key == NULL;
    
    if (off != rm) {
        add_to_branches(&a, &b, node_key, off);
    }
    
    for (i = 0; i < sz; ++i) {
        aodbm_data *key = aodbm_read_data(db, pos);
        pos += 4 + key->sz;
        off = aodbm_read64(db, pos);
        pos += 8;
        
        if (!a_placed) {
            if (aodbm_data_lt(a_key, key)) {
                a_placed = true;
                add_to_branches(&a, &b, a_key, node_a);
            }
            if (!b_placed) {
                if (aodbm_data_lt(b_key, key)) {
                    b_placed = true;
                    add_to_branches(&a, &b, b_key, node_b);
                }
            }
        } else {
            if (!b_placed) {
                if (aodbm_data_lt(b_key, key)) {
                    b_placed = true;
                    add_to_branches(&a, &b, b_key, node_b);
                }
            }
        }
        
        if (off != rm) {
            add_to_branches_di(&a, &b, key, off);
        }
    }
    
    if (!a_placed) {
        add_to_branches(&a, &b, a_key, node_a);
    }
    if (!b_placed) {
        add_to_branches(&a, &b, b_key, node_b);
    }
    
    insert_result result;
    if (b.sz + 1 < MAX_NODE_SIZE/2) {
        merge_branches(&a, &b);
        result.a_node = branch_to_rope(&a);
        result.a_key = a.key;
        result.b_node = NULL;
        result.b_key = NULL;
    } else {
        result.a_node = branch_to_rope(&a);
        result.b_node = branch_to_rope(&b);
        result.a_key = a.key;
        result.b_key = b.key;
    }
    return result;
}

aodbm_version aodbm_set(aodbm *db,
                        aodbm_version ver,
                        aodbm_data *key,
                        aodbm_data *val) {
    /* it has to be locked to prevent the append_pos going astray */
    pthread_mutex_lock(&db->rw);
    /* find the position of the appendment (filesize + data block header) */
    uint64_t append_pos = aodbm_file_size(db) + 5;
    aodbm_version result;
    aodbm_data *root = aodbm_data_from_64(ver);
    aodbm_data *append;
    
    if (ver == 0) {
        aodbm_rope *node = aodbm_leaf_node(key, val);
        aodbm_rope_prepend_di(root, node);
        
        append = aodbm_rope_to_data_di(node);
        result = append_pos;
    } else {
        char type;
        aodbm_read(db, ver + 8, 1, &type);
        if (type == 'l') {
            insert_result leaf = insert_into_leaf(db, key, val, ver + 9);
            aodbm_free_data(leaf.a_key);
            if (leaf.b_node == NULL) {
                aodbm_rope *data = leaf.a_node;
                aodbm_rope_prepend_di(root, data);
                
                append = aodbm_rope_to_data_di(data);
                result = append_pos;
            } else {
                size_t a_sz = aodbm_rope_size(leaf.a_node);
                size_t b_sz = aodbm_rope_size(leaf.b_node);
                
                aodbm_rope *data =
                    aodbm_rope_merge_di(leaf.a_node, leaf.b_node);
                size_t data_sz = a_sz + b_sz;
                aodbm_rope *br = aodbm_branch_di(append_pos,
                                                 leaf.b_key,
                                                 append_pos + a_sz);
                aodbm_rope_prepend_di(root, br);
                
                append = aodbm_rope_to_data_di(aodbm_rope_merge_di(data, br));
                result = append_pos + data_sz;
            }
        } else if (type = 'b') {
            aodbm_rope *data = aodbm_rope_empty();
            uint32_t data_sz = 0;
            aodbm_path *path = aodbm_search_path(db, ver, key);
            /* pop the leaf node */
            aodbm_path_node node = aodbm_path_pop(&path);
            insert_result nodes = insert_into_leaf(db, key, val, node.node + 1);
            uint64_t prev_node = node.node;
            
            uint64_t a, b;
            
            while (path != NULL) {
                node = aodbm_path_pop(&path);
                
                uint64_t a_sz, b_sz;
                a_sz = aodbm_rope_size(nodes.a_node);
                a = data_sz + append_pos;
                data = aodbm_rope_merge_di(data, nodes.a_node);
                data_sz += a_sz;
                if (nodes.b_node == NULL) {
                    b = 0;
                } else {
                    b_sz = aodbm_rope_size(nodes.b_node);
                    b = data_sz + append_pos;
                    data = aodbm_rope_merge_di(data, nodes.b_node);
                    data_sz += b_sz;
                }
                
                nodes = insert_into_branch(db,
                                           node.node,
                                           node.key,
                                           a,
                                           nodes.a_key,
                                           b,
                                           nodes.b_key,
                                           prev_node);
                aodbm_free_data(node.key);
                
                prev_node = node.node;
            }
            
            aodbm_free_data(nodes.a_key);
            if (nodes.b_key == NULL) {
                aodbm_rope_prepend_di(root, nodes.a_node);
                
                data = aodbm_rope_merge_di(data, nodes.a_node);
                
                result = data_sz + append_pos;
            } else {
                uint64_t a_sz, b_sz;
                a_sz = aodbm_rope_size(nodes.a_node);
                a = data_sz + append_pos;
                data = aodbm_rope_merge_di(data, nodes.a_node);
                data_sz += a_sz;
                if (nodes.b_node == NULL) {
                    b = 0;
                } else {
                    b_sz = aodbm_rope_size(nodes.b_node);
                    b = data_sz + append_pos;
                    data = aodbm_rope_merge_di(data, nodes.b_node);
                    data_sz += b_sz;
                }
                
                /* create a new branch node */
                aodbm_rope *br = aodbm_branch_di(a, nodes.b_key, b);
                
                aodbm_rope_prepend_di(root, br);
                
                result = data_sz + append_pos;
                
                data = aodbm_rope_merge_di(data, br);
            }
            
            append = aodbm_rope_to_data_di(data);
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

aodbm_version aodbm_del(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        return 0;
    }
    if (!aodbm_has(db, ver, key)) {
        return ver;
    }
    pthread_mutex_lock(&db->rw);
    /* find the position of the appendment (filesize + data block header) */
    uint64_t append_pos = aodbm_file_size(db) + 5;
    
    aodbm_data *append;
    uint64_t result;
    
    aodbm_data *root = aodbm_data_from_64(ver);
    
    char type;
    aodbm_read(db, ver + 8, 1, &type);
    if (type == 'l') {
        remove_result res = remove_from_leaf(db, key, ver + 9);
        if (res.key != NULL) {
            /* empty leaves don't have a key */
            aodbm_free_data(res.key);
        }
        
        aodbm_rope_prepend_di(root, res.data);
        
        append = aodbm_rope_to_data_di(res.data);
        result = append_pos;
    } else if (type == 'b') {
        /* TODO: modify to merge nodes */
        aodbm_rope *data = aodbm_rope_empty();
        uint64_t data_sz = 0;
        aodbm_path *path = aodbm_search_path(db, ver, key);
        
        /* pop the leaf node */
        aodbm_path_node node = aodbm_path_pop(&path);
        remove_result leaf = remove_from_leaf(db, key, node.node + 1);
        /* XXX: this is a hack, TODO: clean up the code that modifies nodes */
        insert_result nodes = { leaf.data, leaf.key, NULL, NULL };
        uint64_t prev_node = node.node;
        
        uint64_t a, b;
        
        while (path != NULL) {
            node = aodbm_path_pop(&path);
            
            uint64_t a_sz, b_sz;
            a_sz = aodbm_rope_size(nodes.a_node);
            a = data_sz + append_pos;
            data = aodbm_rope_merge_di(data, nodes.a_node);
            data_sz += a_sz;
            if (nodes.b_node == NULL) {
                b = 0;
            } else {
                b_sz = aodbm_rope_size(nodes.b_node);
                b = data_sz + append_pos;
                data = aodbm_rope_merge_di(data, nodes.b_node);
                data_sz += b_sz;
            }
        
            nodes = insert_into_branch(db,
                                       node.node,
                                       node.key,
                                       a,
                                       nodes.a_key,
                                       b,
                                       nodes.b_key,
                                       prev_node);
            aodbm_free_data(node.key);
            
            prev_node = node.node;
        }
        
        aodbm_free_data(nodes.a_key);
        if (nodes.b_key == NULL) {
            aodbm_rope_prepend_di(root, nodes.a_node);
            
            data = aodbm_rope_merge_di(data, nodes.a_node);
            
            result = data_sz + append_pos;
        } else {
            uint64_t a_sz, b_sz;
            a_sz = aodbm_rope_size(nodes.a_node);
            a = data_sz + append_pos;
            data = aodbm_rope_merge_di(data, nodes.a_node);
            data_sz += a_sz;
            if (nodes.b_node == NULL) {
                b = 0;
            } else {
                b_sz = aodbm_rope_size(nodes.b_node);
                b = data_sz + append_pos;
                data = aodbm_rope_merge_di(data, nodes.b_node);
                data_sz += b_sz;
            }
            
            /* create a new branch node */
            aodbm_rope *br = aodbm_branch_di(a, nodes.b_key, b);
            
            aodbm_rope_prepend_di(root, br);
            
            result = data_sz + append_pos;
            
            data = aodbm_rope_merge_di(data, br);
        }
        
        append = aodbm_rope_to_data_di(data);
    } else {
        printf("unknown node type\n");
        exit(1);
    }
    
    aodbm_write_data_block(db, append);
    aodbm_free_data(append);
    
    pthread_mutex_unlock(&db->rw);
    return result;
}

bool aodbm_has(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        return false;
    }
    uint64_t leaf_node = aodbm_search(db, ver, key);
    uint32_t sz = aodbm_read32(db, leaf_node + 1);
    uint32_t i;
    uint64_t pos = leaf_node + 5;
    for (i = 0; i < sz; ++i) {
        aodbm_data *r_key = aodbm_read_data(db, pos);
        pos += r_key->sz + 4;
        aodbm_data *r_val = aodbm_read_data(db, pos);
        pos += r_val->sz + 4;
        
        bool eq = aodbm_data_eq(key, r_key);
        
        aodbm_free_data(r_key);
        aodbm_free_data(r_val);
        
        if (eq) {
            return true;
        }
    }
    return false;
}

aodbm_data *aodbm_get(aodbm *db, aodbm_version ver, aodbm_data *key) {
    if (ver == 0) {
        return NULL;
    }
    uint64_t leaf_node = aodbm_search(db, ver, key);
    uint32_t sz = aodbm_read32(db, leaf_node + 1);
    uint32_t i;
    uint64_t pos = leaf_node + 5;
    for (i = 0; i < sz; ++i) {
        aodbm_data *r_key = aodbm_read_data(db, pos);
        pos += r_key->sz + 4;
        aodbm_data *r_val = aodbm_read_data(db, pos);
        pos += r_val->sz + 4;
        
        bool eq = aodbm_data_eq(key, r_key);
        
        aodbm_free_data(r_key);
        
        if (eq) {
            return r_val;
        }
        
        aodbm_free_data(r_val);
    }
    return NULL;
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
    return aodbm_is_based_on(db, aodbm_previous_version(db, a), b);
}

aodbm_version aodbm_previous_version(aodbm *db, aodbm_version ver) {
    return aodbm_read64(db, ver);
}
