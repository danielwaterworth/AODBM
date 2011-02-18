srcs = aodbm.c aodbm_data.c aodbm_rope.c aodbm_internal.c
objs = aodbm.o aodbm_data.o aodbm_rope.o aodbm_internal.o

all:
	gcc ${srcs} -fPIC -c -I./ -D_GNU_SOURCE -g
	rm libaodbm.a
	ar -cq libaodbm.a ${objs}
	gcc ${objs} -shared -o libaodbm.so -g
	gcc aodbm_test.c libaodbm.a -o aodbm_test -I./ -lpthread -g
