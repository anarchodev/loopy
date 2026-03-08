#ifndef loopy_internal_h_INCLUDED
#define loopy_internal_h_INCLUDED

#include <serve/serve.h>
#include "allocator/allocator.h"
#include "kv/kv.h"

typedef struct {
  const char *host;
  int port;
  int backlog;
  int reset;
} loopy_options_t;

loopy_options_t loopy_parse_args(int argc, char **argv);
int loopy_load_mjs(allocator_t allocator, kv_i kv, const char *dir_path);

#endif /* loopy_internal_h_INCLUDED */
