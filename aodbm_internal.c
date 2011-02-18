#include "stdio.h"

#include "aodbm_internal.h"

void print_hex(char c) {
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
