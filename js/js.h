#ifndef js_h_INCLUDED
#define js_h_INCLUDED

#include "kv/kv.h"
#include "serve/serve.h"

typedef struct {
  kv_i kv;
  srv_request_i request;
  srv_response_i response;
} js_options_t;

void js_run(js_options_t options);

#endif
