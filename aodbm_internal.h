#ifndef AODBM_INTERNAL
#define AODBM_INTERNAL

#include "aodbm.h"

struct aodbm {
    #ifdef AODBM_USE_MMAP
    int fd;
    #else
    FILE *fd;
    #endif
    pthread_mutex_t rw;
    volatile uint64_t cur;
    pthread_mutex_t version;
};

void print_hex(unsigned char);
void annotate_data(const char *name, aodbm_data *);
void annotate_rope(const char *name, aodbm_rope *);

aodbm_rope *make_block(aodbm_data *);
aodbm_rope *make_block_di(aodbm_data *);
aodbm_rope *make_record(aodbm_data *, aodbm_data *);
aodbm_rope *make_record_di(aodbm_data *, aodbm_data *);

bool aodbm_read_bytes(aodbm *, void *, size_t);
bool aodbm_seek(aodbm *, size_t, int);
uint64_t aodbm_tell(aodbm *);
void aodbm_write_bytes(aodbm *, void *, size_t);

void aodbm_write_data_block(aodbm *db, aodbm_data *data);
void aodbm_write_version(aodbm *db, uint64_t ver);
void aodbm_read(aodbm *db, uint64_t off, size_t sz, void *ptr);
uint32_t aodbm_read32(aodbm *db, uint64_t off);
uint64_t aodbm_read64(aodbm *db, uint64_t off);
aodbm_data *aodbm_read_data(aodbm *db, uint64_t off);

/* returns the offset of the leaf node that the key belongs in */
uint64_t aodbm_search(aodbm *, aodbm_version, aodbm_data *);

struct aodbm_path_node {
    aodbm_data *key;
    uint64_t node;
};

typedef struct aodbm_path_node aodbm_path_node;

struct aodbm_path;
typedef struct aodbm_path aodbm_path;

void aodbm_path_push(aodbm_path **, aodbm_path_node);
aodbm_path_node aodbm_path_pop(aodbm_path **);
void aodbm_path_print(aodbm_path *);

aodbm_path *aodbm_search_path(aodbm *, aodbm_version, aodbm_data *);

#endif
