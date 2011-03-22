srcs = aodbm.c aodbm_data.c aodbm_rope.c aodbm_internal.c aodbm_rwlock.c \
       aodbm_stack.c aodbm_hash.c aodbm_list.c aodbm_changeset.c
objs = aodbm.o aodbm_data.o aodbm_rope.o aodbm_internal.o aodbm_rwlock.o \
       aodbm_stack.o aodbm_hash.o aodbm_list.o aodbm_changeset.o
flags = -g -fPIC -lpthread -D_FILE_OFFSET_BITS=64 #-DAODBM_USE_MMAP
test_srcs = c_tests/hash_test.c c_tests/data_test.c c_tests/rope_test.c \
            c_tests/stack_test.c c_tests/rwlock_test.c c_tests/list_test.c \
            c_tests/changeset_test.c

all:
	gcc ${srcs} -c -I./ -D_GNU_SOURCE ${flags}
	rm libaodbm.a -f
	ar -cq libaodbm.a ${objs}
	gcc ${objs} -shared -o libaodbm.so ${flags}

check: all
	gcc ${test_srcs} aodbm_test.c libaodbm.a -o run_c_tests ${flags} -lcheck \
	-lpthread -I./c_tests/ -I./
	python aodbm_test.py
	./run_c_tests
