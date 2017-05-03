CFLAGS += -std=c99 -Wall -pedantic
#Flags given by the teacher to use for testing purpuses (There is actually some output with them)
#CFLAGS += -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast 
#CFLAGS += -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wunreachable-code

all: test-inodes test-file test-dirent test-direntlookup shell fs test-bitmap

inode.o: inode.c inode.h

error.o: error.c error.h

bmblock.o: bmblock.c error.h

test-core.o: test-core.c

test-machin.o: test-machin.c

test-machin: test-machin.o test-core.o error.o mount.o sector.o inode.o
	gcc -o $@ $^
	
test-bitmap.o: test-bitmap.c

test-bitmap: test-bitmap.o error.o bmblock.o 
	gcc -o $@ $^

test-inodes.o: test-inodes.c

test-inodes: test-inodes.o test-core.o error.o mount.o sector.o inode.o bmblock.o
	gcc -o $@ $^

test-file.o: test-file.c

test-file : test-file.o test-core.o filev6.o error.o mount.o sector.o inode.o sha.o bmblock.o
	gcc -o $@ $^ -lcrypto

test-dirent.o: test-dirent.c

test-dirent: test-dirent.o test-core.o mount.o error.o direntv6.o sector.o filev6.o inode.o bmblock.o
	gcc -o $@ $^
	
test-direntlookup.o: test-direntlookup.c

test-direntlookup: test-direntlookup.o test-core.o mount.o error.o direntv6.o sector.o filev6.o inode.o bmblock.o
	gcc -o $@ $^

shell.o: shell.c

shell: shell.o mount.o sector.o direntv6.o error.o inode.o sha.o filev6.o bmblock.o
	gcc -g -o $@ $^ -lcrypto

direntv6.o: direntv6.c direntv6.h

sector.o: sector.c sector.h

mount.o: mount.c mount.h

filev6.o: filev6.c mount.h

sha.o: sha.c sha.h

fs.o: fs.c
	$(COMPILE.c) -D_DEFAULT_SOURCE $$(pkg-config fuse --cflags) -o $@ -c $<

fs: fs.o mount.o sector.o direntv6.o error.o inode.o filev6.o bmblock.o
	$(LINK.c) -o $@ $^ $(LDLIBS) $$(pkg-config fuse --libs)

clean:
	rm -f *.o

erase:
	rm -f test-machin test-inodes test-file test-dirent test-direntlookup shell fs test-bitmap
