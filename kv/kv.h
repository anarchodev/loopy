#ifndef kv_h_INCLUDED
#define kv_h_INCLUDED

#include "allocator/allocator.h"
#include "str/str.h"

typedef struct kv_s *kv_i;

int kv_init(allocator_t allocator, kv_i self);
void kv_deinit(kv_i self);

int kv_get(kv_i self, str_slice_t key, str_t*result);
int kv_set(kv_i self, str_slice_t key, str_slice_t val);

#endif
