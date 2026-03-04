#ifndef js_internal_h_INCLUDED
#define js_internal_h_INCLUDED

#include "kv/kv.h"
#include <quickjs.h>

struct js_s {
    allocator_t allocator;
    JSRuntime*runtime;
    JSContext*context;
    kv_i kv;
};

#endif
