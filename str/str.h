#ifndef str_h_INCLUDED
#define str_h_INCLUDED

#include "allocator/allocator.h"
#include <stddef.h>

typedef struct {
  char *ptr;
  unsigned long len;
} str_slice_t;

typedef struct {
  str_slice_t s;
  unsigned long capacity;
} str_fixed_t;

typedef struct str_cls_s*str_cls_i;

typedef struct {
  str_slice_t slice;
  union {
    unsigned int capacity;
    char buffer[16];
  } storage;
} str_t;

#define to_slice(s) str_cstring_to_slice((s), (sizeof(s)) - 1)

#define MAX_DYNAMIC_BUFFER (1024 * 1024)

str_slice_t str_cstring_to_slice(const char *const cstring, unsigned long len);
str_slice_t str_slice(char *ptr, unsigned long len);
int str_slice_eq(str_slice_t a, str_slice_t b);

int str_init(allocator_t t, str_t *const self, str_slice_t str);
void str_clear(allocator_t t, str_t *const self);
void str_deinit(allocator_t t, str_t *const self);
int str_append(allocator_t allocator, str_t *const self, str_slice_t str);
int str_append_long(allocator_t allocator, str_t *const self, long num);
void str_to_cstring(str_t *const self, char *out);
void str_reset(str_t *const self);

str_fixed_t str_fixed_init(str_slice_t str, unsigned long capacity);
int str_fixed_append(str_fixed_t *const self, str_slice_t slice);
int str_fixed_append_long(str_fixed_t *const self, long data);
str_slice_t str_fixed_get_slice(str_fixed_t const*const self);
unsigned long str_fixed_len(str_fixed_t const*const self);
/* out must be at least self->s.len + 1 bytes.
 * When using the same array as the backing buffer, initialize with
 * sizeof buf - 1 capacity to leave room for the NUL terminator. */
void str_fixed_to_cstring(str_fixed_t const *self, char *out);

#endif
