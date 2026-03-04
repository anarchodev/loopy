#ifndef js_internal_h_INCLUDED
#define js_internal_h_INCLUDED

#include "kv/kv.h"
#include <quickjs.h>

typedef struct {
    allocator_t allocator;
    JSRuntime*runtime;
    JSContext*context;
    kv_i kv;
}js_t;

#endif
