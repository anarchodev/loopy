#ifndef js_internal_h_INCLUDED
#define js_internal_h_INCLUDED

#include "js/js.h"
#include "kv/kv.h"
#include "str/str.h"
#include <quickjs.h>

typedef struct {
    allocator_t allocator;
    JSRuntime*runtime;
    JSContext*context;
    kv_i kv;
    str_t kv_prefix;
}js_t;

js_t *js_new(allocator_t, js_options_t);
void js_delete(js_t *);

#endif
