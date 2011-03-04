srcs = aodbm.c aodbm_data.c aodbm_rope.c aodbm_internal.c aodbm_rwlock.c
objs = aodbm.o aodbm_data.o aodbm_rope.o aodbm_internal.o aodbm_rwlock.o
flags = -g -lpthread -D_FILE_OFFSET_BITS=64 #-DAODBM_USE_MMAP

all:
	gcc ${srcs} -fPIC -c -I./ -D_GNU_SOURCE ${flags}
	rm libaodbm.a -f
	ar -cq libaodbm.a ${objs}
	gcc ${objs} -shared -o libaodbm.so ${flags}
