#ifndef AODBM
#define AODBM

#include <stdbool.h>
#include <stdint.h>

/* the type of the database object */
struct aodbm;

/* the main type for data */
struct aodbm_data {
    char *dat;
    size_t sz;
};

typedef struct aodbm aodbm;
typedef struct aodbm_data aodbm_data;

/* this type is used to make merging aodbm_data objects cheaper */
/* implementation note:
     internally it is just a list of aodbm_data objects,
     rather than the more usual tree of strings */
struct aodbm_rope;
typedef struct aodbm_rope aodbm_rope;

typedef uint64_t aodbm_version;

/* database functions */
aodbm *aodbm_open(const char *);
void aodbm_close(aodbm *);

uint64_t aodbm_file_size(aodbm *);

aodbm_version aodbm_current(aodbm *);
bool aodbm_commit(aodbm *, aodbm_version);

bool aodbm_has(aodbm *, aodbm_version, aodbm_data *);
aodbm_version aodbm_set(aodbm *, aodbm_version, aodbm_data *, aodbm_data *);
aodbm_data *aodbm_get(aodbm *, aodbm_version, aodbm_data *);
aodbm_version aodbm_del(aodbm *, aodbm_version, aodbm_data *);
bool aodbm_is_based_on(aodbm *, aodbm_version, aodbm_version);

/* NOTE: functions ending in di have destructive input */

/* aodbm_data functions */
aodbm_data *aodbm_construct_data(const char *, size_t);
aodbm_data *aodbm_data_dup(aodbm_data *);
aodbm_data *aodbm_data_from_str(const char *);
aodbm_data *aodbm_data_from_32(uint32_t);
aodbm_data *aodbm_data_from_64(uint64_t);
aodbm_data *aodbm_cat_data(aodbm_data *, aodbm_data *);
aodbm_data *aodbm_cat_data_di(aodbm_data *, aodbm_data *);
aodbm_data *aodbm_data_empty();
aodbm_data *aodbm_data_dup(aodbm_data *);

bool aodbm_data_lt(aodbm_data *, aodbm_data *);
bool aodbm_data_gt(aodbm_data *, aodbm_data *);
bool aodbm_data_lte(aodbm_data *, aodbm_data *);
bool aodbm_data_gte(aodbm_data *, aodbm_data *);
bool aodbm_data_eq(aodbm_data *, aodbm_data *);
int aodbm_data_cmp(aodbm_data *, aodbm_data *);

void aodbm_free_data(aodbm_data *);

/* data printing */
void aodbm_print_data(aodbm_data *);
void aodbm_print_rope(aodbm_rope *);

/* aodbm_rope functions */
aodbm_rope *aodbm_rope_empty();
aodbm_rope *aodbm_data_to_rope_di(aodbm_data *);
aodbm_rope *aodbm_data_to_rope(aodbm_data *);
aodbm_rope *aodbm_data2_to_rope_di(aodbm_data *, aodbm_data *);
aodbm_rope *aodbm_data2_to_rope(aodbm_data *, aodbm_data *);
size_t aodbm_rope_size(aodbm_rope *);
aodbm_data *aodbm_rope_to_data(aodbm_rope *);
aodbm_data *aodbm_rope_to_data_di(aodbm_rope *);
void aodbm_free_rope(aodbm_rope *);

/* the aodbm_data object is destroyed */
void aodbm_rope_append_di(aodbm_rope *, aodbm_data *);
void aodbm_rope_prepend_di(aodbm_data *, aodbm_rope *);

void aodbm_rope_append(aodbm_rope *, aodbm_data *);
void aodbm_rope_prepend(aodbm_data *, aodbm_rope *);
aodbm_rope *aodbm_rope_merge_di(aodbm_rope *, aodbm_rope *);

#endif
