#ifndef js_h_INCLUDED
#define js_h_INCLUDED

#include "kv/kv.h"
#include "serve/serve.h"
#include "str/str.h"

typedef struct js_s *js_i;
typedef void (*js_eval_cb)(js_i self, str_t *body, int status);

int js_init(allocator_t allocator, kv_i kv, srv_request_i request,
            srv_response_i response, js_i self);

void js_eval(js_i self, str_slice_t module, str_slice_t function, int argc,
             str_slice_t argv[]);

#endif
