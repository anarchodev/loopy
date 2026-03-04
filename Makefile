.PHONY: all clean loopy test coverage

BUILD ?= debug

ifeq ($(BUILD),release)
  CFLAGS_MODE  = -O2 -DNDEBUG
  LDFLAGS_MODE =
else
  CFLAGS_MODE  = -g3 -fsanitize=address,undefined -O0 -fanalyzer --coverage -Werror
  LDFLAGS_MODE = -fsanitize=address,undefined --coverage -lgcov
endif

CFLAGS  = -Wextra -pedantic -std=c89 -Wall -D_GNU_SOURCE=1  \
          -I. $(CFLAGS_MODE)

LDFLAGS = -fuse-ld=mold $(LDFLAGS_MODE)

all: loopy

clean:
	find . -type f \( -name "*.o" -o -name "*.gcda" -o -name "*.gcno" \) \
	       -not -path "./vendor*" -exec rm {} \;
	rm -rf bin/* coverage.info coverage_html

## vendor
UV_A = vendor/libuv/build/libuv.a
PQ_A = vendor/postgres/src/interfaces/libpq/libpq.a
QUICKJS_A = vendor/quickjs/build/libqjs.a
QUICKJS_LIBC_A = vendor/quickjs/build/libqjs-libc.a
SQLITE_A = vendor/sqlite/libsqlite3.a

vendor/sqlite/configure:
	git submodule update --init --recursive

$(UV_A):
	cd vendor/libuv && cmake -B build && cmake --build build

$(QUICKJS_A):
	cd vendor/quickjs && cmake -B build && cmake --build build

$(SQLITE_A): vendor/sqlite/configure
	cd vendor/sqlite && ./configure && $(MAKE) MAKELEVEL=0

$(PQ_A):
	cd vendor/postgres && ./configure && $(MAKE) -C src/interfaces/libpq MAKELEVEL=0

vendor/picohttpparser/%: CFLAGS=-I. -std=gnu23 -Ivendor/libuv/include \
                         -Ivendor/picohttpparser $(CFLAGS_MODE)

# utility code
LOG_OBJS = log/log.o

ALLOCATOR_OBJS = allocator/allocator.o

STR_OBJS = str/str.o str/fixed.o
$(STR_OBJS): str/str.h

# wrapper code
serve/%: CFLAGS=-I. -std=gnu23 -Ivendor/libuv/include -Ivendor/picohttpparser $(CFLAGS_MODE)
SERVE_OBJS = serve/serve.o \
             vendor/picohttpparser/picohttpparser.o \
             serve/req.o \
             serve/res.o
$(SERVE_OBJS): $(UV_A) serve/serve.h serve/serve_internal.h

kv/%: CFLAGS=-I. -std=gnu23 -Ivendor/sqlite $(CFLAGS_MODE)
KV_OBJS = kv/kv.o
$(KV_OBJS): kv/kv.h

js/%: CFLAGS=-I. -std=gnu23 -Ivendor/quickjs $(CFLAGS_MODE)
JS_OBJS=js/js.o
$(JS_OBJS): $(QUICKJS_A) js/js.h js/js_internal.h

## programs
LOOPY_OBJS = loopy/main.o loopy/args.o
$(LOOPY_OBJS): loopy/internal.h

loopy: bin/loopy
             
bin/loopy: $(LOOPY_OBJS) $(KV_OBJS) $(ALLOCATOR_OBJS) $(STR_OBJS) \
	   $(SERVE_OBJS) $(LOG_OBJS) $(UV_A) $(QUICKJS_A) $(SQLITE_A)
	$(CC) $(LDFLAGS) $^ -lm -o $@

LOOPY_TEST_OBJS = loopy/test.o

loopy-test: bin/loopy-test

bin/loopy-test: $(LOOPY_TEST_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

str-test: bin/str-test

STR_TEST_OBJS = str/test.o
$(STR_TEST_OBJS): str/str.h 
bin/str-test: $(STR_TEST_OBJS) $(STR_OBJS) $(ALLOCATOR_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

test: loopy-test str-test kv-test js-test serve-test
	./bin/loopy-test
	./bin/str-test
	./bin/kv-test
	./bin/js-test
	./bin/serve-test

coverage: test
	./bin/str-test
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
	@echo "open coverage_html/index.html"

SERVE_TEST_OBJS = serve/test.o
$(SERVE_TEST_OBJS): serve/serve.h serve/serve_internal.h

serve-test: bin/serve-test

bin/serve-test: $(SERVE_TEST_OBJS) serve/req.o serve/res.o \
	        vendor/picohttpparser/picohttpparser.o \
	        $(STR_OBJS) $(ALLOCATOR_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

JS_TEST_OBJS = js/test.o
$(JS_TEST_OBJS): js/js.h js/js_internal.h

js-test: bin/js-test

bin/js-test: $(JS_TEST_OBJS) $(JS_OBJS) $(KV_OBJS) $(SQLITE_A) $(STR_OBJS) \
	     $(ALLOCATOR_OBJS) $(QUICKJS_A) serve/req.o serve/res.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

KV_TEST_OBJS = kv/test.o
$(KV_TEST_OBJS): kv/kv.h kv/kv_internal.h

kv-test: ./bin/kv-test

bin/kv-test: $(KV_TEST_OBJS) $(KV_OBJS) $(SQLITE_A) $(STR_OBJS) \
	     $(ALLOCATOR_OBJS)
	$(CC) $(LDFLAGS) $^ -lm -o $@
