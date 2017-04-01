CFLAGS += -std=c99 -Wall -pedantic 
#Flags given by the teacher to use for testing purpuses (There is actually some output with them)
#CFLAGS += -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast 
#CFLAGS += -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wunreachable-code

all: test-inodes test-file

inode.o: inode.c inode.h sector.o error.o

error.o: error.c error.h

test-core.o: test-core.c error.o test-dirent.o test-inodes.o mount.o 

test-machin.o: test-machin.c error.o

test-machin: test-machin.o test-core.o error.o mount.o sector.o inode.o
	gcc -o $@ $^

test-inodes.o: test-inodes.c test-inodes.h mount.o error.o

test-inodes: test-inodes.o test-core.o error.o mount.o sector.o inode.o
	gcc -o $@ $^

test-file.o: test-file.c test-file.h test-core.o filev6.o error.o mount.o sector.o inode.o

test-file : test-file.o test-core.o filev6.o error.o mount.o sector.o inode.o sha.o
	gcc -o $@ $^ -lcrypto

test-dirent.o: test-dirent.c test-dirent.h direntv6.o mount.o error.o sector.o

test-dirent: test-core.o mount.o test-inodes.o test-dirent.o error.o
	gcc -o $@ $^

direntv6.o: direntv6.c direntv6.h filev6.o error.o

sector.o: sector.c sector.h error.o

mount.o: mount.c mount.h sector.o inode.o error.o

filev6.o: filev6.c filev6.h mount.o inode.o error.o

sha.o: sha.c sha.h inode.o mount.o filev6.o

clean:
	rm -f *.o

erase:
	rm -f test-machin test-inodes test-file test-dirent
