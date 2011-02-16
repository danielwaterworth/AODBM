#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "aodbm.h"

const char *bool_to_str(bool a) {
    return a?"true":"false";
}

int main() {
    aodbm_data *key = aodbm_data_from_str("hello");
    aodbm_data *val = aodbm_data_from_str("world");
    
    aodbm_data *key1 = aodbm_data_from_str("hello1");
    aodbm_data *val1 = aodbm_data_from_str("world1");

    aodbm *db = aodbm_open("testdb");
    
    aodbm_version ver = aodbm_current(db);
    
    aodbm_version new_ver = aodbm_set(db, ver, key, val);
    printf("written first set\n");
    //new_ver = aodbm_set(db, new_ver, key1, val1);
    //printf("written second set\n");
    
    aodbm_data *test = aodbm_get(db, new_ver, key);
    
    aodbm_print_data(test);
    printf("\n");
    
    aodbm_close(db);
    
    return 0;
}
