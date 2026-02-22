#ifndef js_internal_h_INCLUDED
#define js_internal_h_INCLUDED

#include "allocator/allocator.h"
#include "js.h"
#include "quickjs.h"

struct js_s {
    JSRuntime*runtime;
    JSContext*context;
    allocator_t allocator;
    void*opaque;
};

#endif
