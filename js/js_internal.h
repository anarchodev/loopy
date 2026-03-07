#ifndef js_internal_h_INCLUDED
#define js_internal_h_INCLUDED

#include "kv/kv.h"
#include "serve/serve.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#endif

#include <quickjs.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

typedef struct js_s {
    allocator_t allocator;
    JSRuntime*runtime;
    JSContext*context;
    kv_i kv;
    srv_request_i request;
    srv_response_i response;
}js_t;

#endif
