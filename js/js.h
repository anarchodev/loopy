#ifndef js_h_INCLUDED
#define js_h_INCLUDED

#include "kv/kv.h"
#include "serve/serve.h"

typedef struct {
  kv_i kv;
  srv_request_i request;
  srv_response_i response;
  str_t kv_prefix;
} js_options_t;

int js_run(allocator_t allocator, js_options_t options);

#endif
