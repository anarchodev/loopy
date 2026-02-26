.PHONY: all clean loopy test
	
CFLAGS = -Wextra -pedantic -g3 \
         -fsanitize=address,undefined -O0 -std=c89\
         -Wall -D_GNU_SOURCE=1 -Werror -fanalyzer \
         -fuse-ld=mold -I.

all: loopy

clean:
	find . -type f -name "*.o" -not -path "./vendor*" -exec rm {} \;
	rm bin/*

## vendor
UV_A = vendor/libuv/build/libuv.a
PQ_A = vendor/postgres/src/interfaces/libpq/libpq.a
QUICKJS_A = vendor/quickjs/build/libqjs.a
QUICKJS_LIBC_A = vendor/quickjs/build/libqjs-libc.a
SQLITE_A = vendor/sqlite/libsqlite3.a

vendor/sqlite/configure:
	git submodule update --init --recursive

$(UV_A): 
	cd vendor/libuv; cmake -B build; cmake --build build

$(QUICKJS_A):
	cd vendor/quickjs/; cmake -B build; cmake --build build

$(SQLITE_A): vendor/sqlite/configure
	cd vendor/sqlite/; ./configure; make MAKELEVEL=0

$(PQ_A):
	cd vendor/postgres/; ./configure; cd src/interfaces/libpq/; make MAKELEVEL=0

vendor/picohttpparser/%:CFLAGS=-I. -std=gnu23 -Ivendor/libuv/include -Ivendor/picohttpparser

# shared code
serve/%: CFLAGS=-I. -std=gnu23 -Ivendor/libuv/include -Ivendor/picohttpparser -g3
SERVE_OBJS = serve/serve.o \
             vendor/picohttpparser/picohttpparser.o \
             serve/req.o \
             serve/res.o

$(SERVE_OBJS): $(UV_A) serve/serve.h serve/serve_internal.h

js/%: CFLAGS=-I. -std=gnu23 -Ivendor/quickjs -g3

kv/%: CFLAGS=-I. -std=gnu23 -Ivendor/sqlite -g3
KV_OBJS = kv/kv.o

LOG_OBJS = log/log.o

JS_OBJS=js/js.o
$(JS_OBJS): $(QUICKJS_A) js/js.h js/js_internal.h

ALLOCATOR_OBJS = allocator/allocator.o

STR_OBJS = str/str.o
$(STR_OBJS): str/str.h

## programs
LOOPY_OBJS = loopy/main.o loopy/args.o 

$(LOOPY_OBJS): loopy/internal.h

loopy: bin/loopy
             
bin/loopy: $(SQLITE_A) $(QUICKJS_A) $(UV_A) $(LOG_OBJS) $(SERVE_OBJS) $(ALLOCATOR_OBJS) $(LOOPY_OBJS) $(KV_OBJS) $(STR_OBJS) $(JS_OBJS)
	$(CC) $(CFLAGS) $(LOOPY_OBJS) $(SERVE_OBJS) $(ALLOCATOR_OBJS) $(QUICKJS_LIBC_A) $(LOG_OBJS) $(STR_OBJS) $(JS_OBJS) $(KV_OBJS)  $(UV_A) $(QUICKJS_A) $(SQLITE_A) -lm -o $@


LOOPY_TEST_OBJS = loopy/test.o

$(LOOPY_TEST_OBJS): loopy/internal.h

loopy-test: bin/loopy-test

bin/loopy-test: $(LOOPY_TEST_OBJS)
	$(CC) $(CFLAGS) $(LOOPY_TEST_OBJS) -o $@

str-test: bin/str-test

STR_TEST_OBJS = str/str.o str/test.o
$(STR_TEST_OBJS): str/str.h

bin/str-test: $(STR_TEST_OBJS) $(ALLOCATOR_OBJS)
	$(CC) $(CFLAGS) $(STR_TEST_OBJS) $(ALLOCATOR_OBJS) -o $@

test: loopy-test str-test kv-test

KV_TEST_OBJS = kv/test.o kv/kv.o
$(STR_TEST_OBJS): kv/kv.h kv/kv_internal.h

kv-test: ./bin/kv-test

bin/kv-test: $(SQLITE_A) $(KV_TEST_OBJS) $(ALLOCATOR_OBJS) $(STR_OBJS)
	$(CC) $(CFLAGS) $(KV_TEST_OBJS) $(ALLOCATOR_OBJS) $(STR_OBJS) $(SQLITE_A) -lm -o $@
