#ifndef AODBM_INTERNAL
#define AODBM_INTERNAL

#include "aodbm.h"

void print_hex(char);
void annotate_data(const char *name, aodbm_data *);
void annotate_rope(const char *name, aodbm_rope *);

aodbm_rope *make_block(aodbm_data *);
aodbm_rope *make_block_di(aodbm_data *);
aodbm_rope *make_record(aodbm_data *, aodbm_data *);
aodbm_rope *make_record_di(aodbm_data *, aodbm_data *);

#endif
