#ifndef kv_internal_h_INCLUDED
#define kv_internal_h_INCLUDED

#include "allocator/allocator.h"
#include "str/str.h"
#include <sqlite3.h>

typedef struct kv_s {
  allocator_t allocator;
  sqlite3 *db;
} kv_t;

typedef struct kv_result_s {
    str_t *result;
    allocator_t allocator;
} kv_result_t;

#endif
