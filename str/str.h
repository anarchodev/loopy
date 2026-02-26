#ifndef str_h_INCLUDED
#define str_h_INCLUDED

#include "allocator/allocator.h"
#include <stddef.h>

typedef struct str_slice_s {
  char *ptr;
  unsigned long len;
} str_slice_t;

typedef struct str_fixed_s {
  str_slice_t s;
  unsigned long capacity;
} str_fixed_t;

typedef struct str_dynamic_s {
  str_slice_t slice;
  allocator_t allocator;
  union {
    unsigned int capacity;
    char buffer[16];
  } storage;
} str_dynamic_t;

#define to_slice(s) str_cstring_to_slice((s), (sizeof (s))-1)

#define MAX_DYNAMIC_BUFFER (1024 * 1024 * 1024)

str_slice_t str_cstring_to_slice(const char *cstring, unsigned long len);

int str_dynamic_new(allocator_t t, str_dynamic_t *self, str_slice_t str);
void str_dynamic_delete(str_dynamic_t *self);
int str_dynamic_append(str_dynamic_t *self, str_slice_t str);
int str_dynamic_append_long(str_dynamic_t *self, long num);
void str_dynamic_to_cstring(str_dynamic_t *self, char *out);
void str_dynamic_set(str_dynamic_t*self, str_slice_t val);
void str_dynamic_reset(str_dynamic_t *self);

void str_fixed_init(str_fixed_t *self, str_slice_t str, unsigned long capacity);
int str_fixed_append(str_fixed_t *self, str_slice_t slice);
int str_fixed_append_long(str_fixed_t *self, long data);

#endif
