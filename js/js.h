#ifndef js_h_INCLUDED
#define js_h_INCLUDED

#include "kv/kv.h"
#include "serve/serve.h"

typedef struct {
  srv_request_i request;
  srv_response_i response;
  kv_i kv;
} js_options_t;

typedef struct js_s *js_i;

js_i js_new(allocator_t allocator, js_options_t options);
void js_delete(js_i);

int js_run(js_i self);

#endif
