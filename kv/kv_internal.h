#ifndef kv_internal_h_INCLUDED
#define kv_internal_h_INCLUDED

#include "allocator/allocator.h"
#include "str/str.h"
#include <sqlite3.h>

typedef struct kv_s {
  allocator_t allocator;
  sqlite3 *db;
  sqlite3_stmt *get_stmt;
  sqlite3_stmt *set_stmt;
  sqlite3_stmt *remove_stmt;
} kv_t;

#endif
