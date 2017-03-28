CFLAGS = -std=c99 -Wall -pedantic

all: test-inodes

inode.o: inode.c error.o

error.o: error.c

test-core.o: test-core.c error.o

test-machin.o: test-machin.c error.o

test-machin: test-machin.o test-inodes.o error.o mount.o sector.o inode.o
	gcc -o $@ $^

test-inodes.o: test-inodes.c test-core.o error.o

test-inodes: test-inodes.o test-core.o error.o mount.o sector.o inode.o
	gcc -o $@ $^

test-file.o: test-file.c test-inodes.o filev6.o error.o mount.o sector.o inode.o

test-file : test-file.o test-inodes.o filev6.o error.o mount.o sector.o inode.o sha.o
	gcc -o $@ $^ -lcrypto

sector.o: sector.c error.o

mount.o: mount.c error.o

filev6.o: filev6.c mount.o error.o

sha.o: sha.c sha.h inode.o mount.o filev6.o

clean:
	rm -f *.o
