#ifndef js_h_INCLUDED
#define js_h_INCLUDED

#include "kv/kv.h"
#include "serve/serve.h"
#include "str/str.h"

typedef struct js_s *js_i;
typedef void (*js_eval_cb)(js_i self, str_dynamic_t *body, int status);

js_i js_new(allocator_t allocator, kv_i kv);
void js_delete(js_i self);

void js_eval(js_i self, str_slice_t func_path, str_slice_t args, srv_request_i, srv_response_i);
void js_set_opaque(js_i self, void *data);

void *js_get_opaque(js_i self);

#endif
