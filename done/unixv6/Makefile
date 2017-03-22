CFLAGS = -std=c99 -Wall -pedantic

all: test-inodes

inode.o: inode.c error.o

error.o: error.c

test-core.o: test-core.c error.o

test-inodes.o: test-inodes.c test-core.o error.o

test-inodes: test-inodes.o test-core.o error.o mount.o sector.o inode.o
	gcc -o $@ $^

test-file.o: test-file.c test-core.o error.o

test-file : test-file.o test-core.o error.o mount.o sector.o inode.o
	gcc -o $@ $^

sector.o: sector.c error.o

mount.o: mount.c error.o

clean:
	rm -f *.o
