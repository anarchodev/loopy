.PHONY: all clean loopy
	
CFLAGS = -Ivendor/libuv/include -Wextra -pedantic -g3 \
         -fsanitize=address,undefined -O0 -std=gnu23 \
         -Ivendor/sqlite/ -Wall -Ivendor/quickjs -D_GNU_SOURCE=1

all: loopy

clean:
	find . -type f -name "*.o" -not -path "./vendor*" -exec rm {} \;
	rm bin/*

## loopy
LOOPY_OBJS = loopy/loopy.o \
             $(UV_A) \
             vendor/quickjs/libquickjs.a \
             vendor/postgres/src/interfaces/libpq/libpq.a

loopy: bin/loopy

bin/loopy: $(LOOPY_OBJS)
	$(CC) $(CFLAGS) $(LOOPY_OBJS) -o $@

## vendor
UV_A = vendor/libuv/build/libuv.a
PQ_A = vendor/postgres/src/interfaces/libpq/libpq.a
QUICKJS_A = vendor/quickjs/libquickjs.a

UV_A:
	cd vendor/libuv; cmake -B build; cmake --build build

PQ_A:
	cd vendor/quickjs/; make MAKELEVEL=0

QUICKJS_A:
	cd vendor/postgres/; ./configure; cd src/interfaces/libpq/; make MAKELEVEL=0

