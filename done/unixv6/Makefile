CFLAGS = -std=c99 -Wall -pedantic

all: test-inodes

error.o: error.c

test-core.o: test-core.c error.o

test-inodes.o: test-inodes.c test-core.o error.o

test-inodes: test-inodes.o test-core.o error.o
	gcc -o $@ $^

clean:
	rm -f *.o
