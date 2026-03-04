#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

str_fixed_t str_fixed_init(str_slice_t str, unsigned long capacity) {
  str_fixed_t tmp;
  assert(str.len <= capacity);
  tmp.capacity = capacity;
  tmp.s = str;
  return tmp;
}

int str_fixed_append(str_fixed_t *self, str_slice_t slice) {
  if (slice.len > self->capacity - self->s.len) {
    return -1;
  }
  memcpy(self->s.ptr + self->s.len, slice.ptr, slice.len);
  self->s.len += slice.len;
  return 0;
}

/* TODO: all types, and bases? */
int str_fixed_append_long(str_fixed_t *self, long num) {
  char tmp[65];
  int n;
  n = snprintf(tmp, 65, "%ld", num);
  assert(n >= 0);
  return str_fixed_append(self, str_cstring_to_slice(tmp, n));
}

str_slice_t str_fixed_get_slice(str_fixed_t const*const self) { return self->s; }

unsigned long str_fixed_len(str_fixed_t const*const self) { return self->s.len; }
