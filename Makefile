.PHONY: all clean loopy test
	
CFLAGS = -Wextra -pedantic -g3 \
         -fsanitize=address,undefined -O0 -std=c89\
         -Wall -D_GNU_SOURCE=1 -Werror \
         -fuse-ld=mold -I.

all: loopy

clean:
	find . -type f -name "*.o" -not -path "./vendor*" -exec rm {} \;
	rm bin/*

## vendor
UV_A = vendor/libuv/build/libuv.a
PQ_A = vendor/postgres/src/interfaces/libpq/libpq.a
QUICKJS_A = vendor/quickjs/libquickjs.a
SQLITE_A = vendor/sqlite/libsqlite3.a

vendor/sqlite/configure:
	git submodule update --init --recursive

$(UV_A): 
	cd vendor/libuv; cmake -B build; cmake --build build

$(QUICKJS_A):
	cd vendor/quickjs/; make MAKELEVEL=0

$(SQLITE_A): vendor/sqlite/configure
	cd vendor/sqlite/; ./configure; make MAKELEVEL=0

$(PQ_A):
	cd vendor/postgres/; ./configure; cd src/interfaces/libpq/; make MAKELEVEL=0


serve/%: CFLAGS=-I. -std=gnu23 -Ivendor/libuv/include -Ivendor/picohttpparser -g3
vendor/picohttpparser/%:CFLAGS=-I. -std=gnu23 -Ivendor/libuv/include -Ivendor/picohttpparser
SERVE_OBJS = serve/serve.o vendor/picohttpparser/picohttpparser.o
ALLOCATOR_OBJS = allocator/allocator.o
## programs
LOOPY_OBJS = loopy/main.o loopy/args.o  \

$(LOOPY_OBJS): loopy/internal.h

loopy: bin/loopy

             
bin/loopy: $(UV_A) $(SERVE_OBJS) $(ALLOCATOR_OBJS) $(LOOPY_OBJS)
	$(CC) $(CFLAGS) $(LOOPY_OBJS) $(SERVE_OBJS) $(ALLOCATOR_OBJS) $(UV_A) -o $@


LOOPY_TEST_OBJS = loopy/test.o loopy/callbacks.o

$(LOOPY_TEST_OBJS): loopy/internal.h

loopy-test: bin/loopy-test

bin/loopy-test: $(LOOPY_TEST_OBJS)
	$(CC) $(CFLAGS) $(LOOPY_TEST_OBJS) -o $@

test: loopy-test
	./bin/loopy-test

