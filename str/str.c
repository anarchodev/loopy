#include "str.h"
#include "allocator/allocator.h"
#include <stdio.h>
#include <string.h>

static unsigned int new_capacity(unsigned int old) {
  if (old >= MAX_DYNAMIC_BUFFER / 2) {
    return MAX_DYNAMIC_BUFFER;
  }
  return old * 2;
}

void str_clear(allocator_t allocator, str_t *const self) {
  if (self->slice.len > sizeof self->storage.buffer) {
    allocator.free(self->slice.ptr);
    self->slice.ptr = self->storage.buffer;
  }
  self->slice.len = 0;
}

void str_deinit(allocator_t allocator, str_t *const str) {
  if (str->slice.len > sizeof str->storage.buffer) {
    allocator.free(str->slice.ptr);
  }
}

int str_init(allocator_t allocator, str_t *const self, str_slice_t str) {
  if (str.len > MAX_DYNAMIC_BUFFER) {
    return -1;
  }
  if (str.len <= sizeof self->storage.buffer) {
    self->slice.ptr = self->storage.buffer;
    self->slice.len = str.len;
    memcpy(self->storage.buffer, str.ptr, str.len);
  } else {
    self->storage.capacity = new_capacity(str.len);
    if ((self->slice.ptr = allocator.malloc(self->storage.capacity)) == NULL) {
      return -1;
    }
    self->slice.len = str.len;
    memcpy(self->slice.ptr, str.ptr, str.len);
  }
  return 0;
}

int str_append(allocator_t allocator, str_t *const self, str_slice_t str) {
  unsigned int cap;
  char *tmp;

  if (str.len > MAX_DYNAMIC_BUFFER - self->slice.len) {
    return -1;
  }
  if (str.len + self->slice.len <= sizeof self->storage.buffer) {
    goto append;
  }
  if (self->slice.len <= sizeof self->storage.buffer) {
    cap = new_capacity(self->slice.len + str.len);
    self->slice.ptr = allocator.malloc(cap);
    if (self->slice.ptr == NULL) {
      return -1;
    }
    memcpy(self->slice.ptr, self->storage.buffer, self->slice.len);
    self->storage.capacity = cap;
    goto append;
  }
  while (str.len > self->storage.capacity - self->slice.len) {
    self->storage.capacity = new_capacity(self->storage.capacity);
  }
  if ((tmp = allocator.realloc(self->slice.ptr, self->storage.capacity)) == NULL) {
    return -1;
  }
  self->slice.ptr = tmp;

append:
  memcpy(self->slice.ptr + self->slice.len, str.ptr, str.len);
  self->slice.len += str.len;
  return 0;
}

int str_append_long(allocator_t allocator, str_t *self, long num) {
  char tmp[65];
  int n;
  n = snprintf(tmp, 65, "%ld", num);
  if (n < 0) {
    return -1;
  }
  return str_append(allocator, self, str_cstring_to_slice(tmp, n));
}

void str_to_cstring(str_t *self, char *out) {
  memcpy(out, self->slice.ptr, self->slice.len);
  out[self->slice.len] = 0;
}

int str_slice_eq(str_slice_t a, str_slice_t b) {
  if (a.len != b.len) {
    return 0;
  }
  return memcmp(a.ptr, b.ptr, a.len) == 0;
}

str_slice_t str_cstring_to_slice(const char *const cstring, unsigned long len) {
  str_slice_t tmp;
  tmp.len = len;
  tmp.ptr = (char *)cstring;
  return tmp;
}

