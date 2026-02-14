.PHONY: all clean loopy
	
CFLAGS = -Ivendor/libuv/include -Wextra -pedantic -g3 \
         -fsanitize=address,undefined -O0 -std=gnu23 \
         -Ivendor/sqlite/ -Wall -Ivendor/quickjs -D_GNU_SOURCE=1 \
         -fuse-ld=mold

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


## loopy
LOOPY_OBJS = loopy/loopy.o
LOOPY_LIBS = $(SQLITE_A) $(QUICKJS_A) $(PQ_A) $(UV_A)

loopy: bin/loopy

bin/loopy: $(LOOPY_LIBS) $(LOOPY_OBJS)
	$(CC) $(CFLAGS) $(LOOPY_OBJS) $(LOOPY_LIBS) -o $@

