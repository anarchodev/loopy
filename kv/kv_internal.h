#ifndef kv_internal_h_INCLUDED
#define kv_internal_h_INCLUDED

#include "allocator/allocator.h"
#include <sqlite3.h>

typedef struct kv_s {
  allocator_t allocator;
  sqlite3 *db;
} kv_t;

#endif
