all:
	gcc aodbm.c aodbm_data.c aodbm_rope.c -O2 -c -I./ -D_GNU_SOURCE -g
	gcc aodbm_test.c aodbm.o aodbm_data.o aodbm_rope.o -O2 -o aodbm_test -I./ \
	-lpthread -g
