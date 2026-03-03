#ifndef js_internal_h_INCLUDED
#define js_internal_h_INCLUDED

#include "cls/cls.h"
#include "kv/kv.h"
#include "serve/serve.h"
#include <quickjs.h>

struct js_s {
    cls_t cls;
    JSRuntime*runtime;
    JSContext*context;
    kv_i kv;
    srv_request_i request;
    srv_response_i response;
};

#endif
