all:
	gcc aodbm.c -O2 -c -I./ -D_GNU_SOURCE -g
	gcc aodbm_test.c aodbm.o -O2 -o aodbm_test -I./ -lpthread -g
