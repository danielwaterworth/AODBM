#include "stdlib.h"
#include "assert.h"
#include "stdio.h"

#define AODBM_OS_ERROR() \
do { \
printf("\nfailed at %s:%i\n", __FILE__, __LINE__); \
perror("aodbm"); \
assert(0); \
exit(1); \
} while (0);

#define AODBM_CUSTOM_ERROR(msg) \
do { \
printf("\nfailed at %s:%i\nwith message: '%s'\n", __FILE__, __LINE__, msg); \
assert(0); \
exit(1); \
} while (0);
