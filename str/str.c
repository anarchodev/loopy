#include "str.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void str_fixed_init(str_fixed_t *self, str_slice_t str, unsigned long capacity) {
  assert(str.len <= capacity);
  self->s = str;
  self->capacity = capacity;
}

static unsigned int new_capacity(unsigned int old) {
  if (old >= MAX_DYNAMIC_BUFFER / 2) {
    return MAX_DYNAMIC_BUFFER;
  }
  return old * 2;
}

int str_dynamic_new(allocator_t allocator, str_dynamic_t *self,
                    str_slice_t str) {
  if (str.len > MAX_DYNAMIC_BUFFER) {
    return -1;
  }
  self->allocator = allocator;
  if (str.len <= sizeof self->storage.buffer) {
    self->slice.ptr = self->storage.buffer;
    self->slice.len = str.len;
    memcpy(self->storage.buffer, str.ptr, str.len);
  } else {
    self->storage.capacity = new_capacity(self->slice.len);

    self->slice.ptr = allocator.malloc(self->storage.capacity);
    self->slice.len = str.len;
    memcpy(self->slice.ptr, str.ptr, str.len);
  }
  return 0;
}

void str_dyanmic_delete(str_dynamic_t *self) {
  if (self->slice.len > sizeof self->storage.buffer) {
    self->allocator.free(self->slice.ptr);
  }
}

int str_dynamic_append(str_dynamic_t *self, str_slice_t str) {
  if (str.len > MAX_DYNAMIC_BUFFER - self->slice.len) {
    return -1;
  }

  if (str.len + self->slice.len <= sizeof self->storage.buffer) {
    goto append;
  }

  if (self->slice.len <= sizeof self->storage.buffer) {
    self->slice.ptr = self->allocator.malloc(self->slice.len + str.len);
    memcpy(self->slice.ptr, self->storage.buffer, self->slice.len);
    self->storage.capacity = self->slice.len + str.len;
    goto append;
  }

  while (str.len > self->storage.capacity - self->slice.len) {
    self->storage.capacity = new_capacity(self->slice.len);
  }

  self->slice.ptr =
      self->allocator.realloc(self->slice.ptr, self->storage.capacity);

append:
  memcpy(self->slice.ptr + self->slice.len, str.ptr, str.len);
  self->slice.len += str.len;
  return 0;
}

int str_dynamic_append_long(str_dynamic_t *self, long num) {
  char tmp[65];
  int n;
  n = snprintf(tmp, 65, "%ld", num);

  return str_dynamic_append(self, str_cstring_to_slice(tmp, n));
}

void str_dynamic_to_cstring(str_dynamic_t *self, char *out) {
  memcpy(out, self->slice.ptr, self->slice.len);
  out[self->slice.len] = 0;
}

str_slice_t str_cstring_to_slice(const char *cstring, unsigned long len) {
  str_slice_t tmp;
  tmp.len = len;
  tmp.ptr = (char *)cstring;
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
  return str_fixed_append(self, str_cstring_to_slice(tmp, n));
}
