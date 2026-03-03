#include "allocator/allocator.h"
#include "str/str.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static allocator_t allocator;

/* --- str_slice tests --- */

static void test_slice_eq_equal(void) {
  str_slice_t a = str_cstring_to_slice("hello", 5);
  str_slice_t b = str_cstring_to_slice("hello", 5);
  assert(str_slice_eq(a, b));
}

static void test_slice_eq_different_content(void) {
  str_slice_t a = str_cstring_to_slice("hello", 5);
  str_slice_t b = str_cstring_to_slice("world", 5);
  assert(!str_slice_eq(a, b));
}

static void test_slice_eq_different_length(void) {
  str_slice_t a = str_cstring_to_slice("hi", 2);
  str_slice_t b = str_cstring_to_slice("hello", 5);
  assert(!str_slice_eq(a, b));
}

static void test_slice_eq_empty(void) {
  str_slice_t a = str_cstring_to_slice("", 0);
  str_slice_t b = str_cstring_to_slice("", 0);
  assert(str_slice_eq(a, b));
}

/* --- str_t init tests --- */

static void test_str_init_small_is_inline(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("hello", 5)) == 0);
  assert(s.slice.len == 5);
  assert(s.slice.ptr == s.storage.buffer);
  assert(memcmp(s.slice.ptr, "hello", 5) == 0);
  str_deinit(allocator, &s);
}

static void test_str_init_at_sso_boundary(void) {
  /* exactly 16 bytes stays inline */
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("1234567890123456", 16)) == 0);
  assert(s.slice.len == 16);
  assert(s.slice.ptr == s.storage.buffer);
  str_deinit(allocator, &s);
}

static void test_str_init_heap(void) {
  /* 17 bytes goes to heap */
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("12345678901234567", 17)) == 0);
  assert(s.slice.len == 17);
  assert(s.slice.ptr != s.storage.buffer);
  assert(memcmp(s.slice.ptr, "12345678901234567", 17) == 0);
  str_deinit(allocator, &s);
}

/* --- str_t append tests --- */

static void test_str_append_stays_inline(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("ab", 2)) == 0);
  assert(str_append(allocator, &s, str_cstring_to_slice("cd", 2)) == 0);
  assert(s.slice.len == 4);
  assert(s.slice.ptr == s.storage.buffer);
  assert(memcmp(s.slice.ptr, "abcd", 4) == 0);
  str_deinit(allocator, &s);
}

static void test_str_append_crosses_sso(void) {
  str_t s;
  /* "hello world!!" = 13 bytes, still inline */
  assert(str_init(allocator, &s, str_cstring_to_slice("hello", 5)) == 0);
  assert(str_append(allocator, &s, str_cstring_to_slice(" world!!", 8)) == 0);
  assert(s.slice.ptr == s.storage.buffer);
  assert(s.slice.len == 13);
  /* append 4 more -> 17 bytes, transitions to heap */
  assert(str_append(allocator, &s, str_cstring_to_slice("XXXX", 4)) == 0);
  assert(s.slice.len == 17);
  assert(s.slice.ptr != s.storage.buffer);
  assert(memcmp(s.slice.ptr, "hello world!!XXXX", 17) == 0);
  str_deinit(allocator, &s);
}

static void test_str_append_heap_grows(void) {
  str_t s;
  int i;
  assert(str_init(allocator, &s, str_cstring_to_slice("12345678901234567", 17)) == 0);
  for (i = 0; i < 10; i++) {
    assert(str_append(allocator, &s, str_cstring_to_slice("aaaa", 4)) == 0);
  }
  assert(s.slice.len == 57);
  str_deinit(allocator, &s);
}

static void test_str_append_long_positive(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("n=", 2)) == 0);
  assert(str_append_long(allocator, &s, 42) == 0);
  assert(s.slice.len == 4);
  assert(memcmp(s.slice.ptr, "n=42", 4) == 0);
  str_deinit(allocator, &s);
}

static void test_str_append_long_negative(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("", 0)) == 0);
  assert(str_append_long(allocator, &s, -100) == 0);
  assert(s.slice.len == 4);
  assert(memcmp(s.slice.ptr, "-100", 4) == 0);
  str_deinit(allocator, &s);
}

static void test_str_append_long_zero(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("", 0)) == 0);
  assert(str_append_long(allocator, &s, 0) == 0);
  assert(s.slice.len == 1);
  assert(memcmp(s.slice.ptr, "0", 1) == 0);
  str_deinit(allocator, &s);
}

static void test_str_append_overflow(void) {
  str_t s;
  char *big = malloc(MAX_DYNAMIC_BUFFER);
  assert(str_init(allocator, &s, str_cstring_to_slice("x", 1)) == 0);
  assert(str_append(allocator, &s, str_cstring_to_slice(big, MAX_DYNAMIC_BUFFER)) == -1);
  assert(s.slice.len == 1);
  free(big);
  str_deinit(allocator, &s);
}

/* --- str_t clear tests --- */

static void test_str_clear_inline(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("abc", 3)) == 0);
  str_clear(allocator, &s);
  assert(s.slice.len == 0);
  assert(s.slice.ptr == s.storage.buffer);
  /* can reuse after clear */
  assert(str_append(allocator, &s, str_cstring_to_slice("hi", 2)) == 0);
  assert(s.slice.len == 2);
  str_deinit(allocator, &s);
}

static void test_str_clear_heap(void) {
  str_t s;
  assert(str_init(allocator, &s, str_cstring_to_slice("12345678901234567", 17)) == 0);
  assert(s.slice.ptr != s.storage.buffer);
  str_clear(allocator, &s);
  assert(s.slice.len == 0);
  assert(s.slice.ptr == s.storage.buffer);
  /* can reuse after clear - should go inline again */
  assert(str_append(allocator, &s, str_cstring_to_slice("abc", 3)) == 0);
  assert(s.slice.ptr == s.storage.buffer);
  assert(s.slice.len == 3);
  str_deinit(allocator, &s);
}

/* --- str_to_cstring --- */

static void test_str_to_cstring(void) {
  str_t s;
  char buf[32];
  assert(str_init(allocator, &s, str_cstring_to_slice("hello", 5)) == 0);
  str_to_cstring(&s, buf);
  assert(buf[5] == '\0');
  assert(strcmp(buf, "hello") == 0);
  str_deinit(allocator, &s);
}

static void test_str_to_cstring_heap(void) {
  str_t s;
  char buf[32];
  assert(str_init(allocator, &s, str_cstring_to_slice("12345678901234567", 17)) == 0);
  str_to_cstring(&s, buf);
  assert(buf[17] == '\0');
  assert(strcmp(buf, "12345678901234567") == 0);
  str_deinit(allocator, &s);
}

/* --- str_fixed tests --- */

static void test_fixed_append_basic(void) {
  char buf[32];
  str_slice_t init;
  str_fixed_t f;
  init.ptr = buf;
  init.len = 0;
  f = str_fixed_init(init, 32);
  assert(str_fixed_append(&f, str_cstring_to_slice("hello", 5)) == 0);
  assert(f.s.len == 5);
  assert(memcmp(buf, "hello", 5) == 0);
  assert(str_fixed_append(&f, str_cstring_to_slice(" world", 6)) == 0);
  assert(f.s.len == 11);
  assert(memcmp(buf, "hello world", 11) == 0);
}

static void test_fixed_append_at_capacity(void) {
  char buf[5];
  str_slice_t init;
  str_fixed_t f;
  init.ptr = buf;
  init.len = 0;
  f = str_fixed_init(init, 5);
  assert(str_fixed_append(&f, str_cstring_to_slice("hello", 5)) == 0);
  assert(f.s.len == 5);
  /* one more byte must fail */
  assert(str_fixed_append(&f, str_cstring_to_slice("x", 1)) == -1);
  assert(f.s.len == 5);
}

static void test_fixed_append_overflow(void) {
  char buf[4];
  str_slice_t init;
  str_fixed_t f;
  init.ptr = buf;
  init.len = 0;
  f = str_fixed_init(init, 4);
  assert(str_fixed_append(&f, str_cstring_to_slice("hi", 2)) == 0);
  /* 3 more won't fit in remaining 2 bytes */
  assert(str_fixed_append(&f, str_cstring_to_slice("xyz", 3)) == -1);
  assert(f.s.len == 2);
}

static void test_fixed_append_long(void) {
  char buf[32];
  str_slice_t init;
  str_fixed_t f;
  init.ptr = buf;
  init.len = 0;
  f = str_fixed_init(init, 32);
  assert(str_fixed_append_long(&f, 12345) == 0);
  assert(f.s.len == 5);
  assert(memcmp(buf, "12345", 5) == 0);
  assert(str_fixed_append_long(&f, -99) == 0);
  assert(f.s.len == 8);
  assert(memcmp(buf, "12345-99", 8) == 0);
}

static void test_fixed_append_long_overflow(void) {
  char buf[3];
  str_slice_t init;
  str_fixed_t f;
  init.ptr = buf;
  init.len = 0;
  f = str_fixed_init(init, 3);
  /* "1234" is 4 chars, won't fit */
  assert(str_fixed_append_long(&f, 1234) == -1);
  assert(f.s.len == 0);
}

int main(void) {
  allocator = allocator_default();

  test_slice_eq_equal();
  test_slice_eq_different_content();
  test_slice_eq_different_length();
  test_slice_eq_empty();

  test_str_init_small_is_inline();
  test_str_init_at_sso_boundary();
  test_str_init_heap();

  test_str_append_stays_inline();
  test_str_append_crosses_sso();
  test_str_append_heap_grows();
  test_str_append_long_positive();
  test_str_append_long_negative();
  test_str_append_long_zero();
  test_str_append_overflow();

  test_str_clear_inline();
  test_str_clear_heap();

  test_str_to_cstring();
  test_str_to_cstring_heap();

  test_fixed_append_basic();
  test_fixed_append_at_capacity();
  test_fixed_append_overflow();
  test_fixed_append_long();
  test_fixed_append_long_overflow();

  return 0;
}
