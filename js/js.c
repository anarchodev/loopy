#include "js.h"
#include "allocator/allocator.h"
#include "js/js_internal.h"
#include "kv/kv.h"
#include "quickjs.h"
#include "str/str.h"
#include <assert.h>

JSModuleDef *kv_module_loader(JSContext *ctx, const char *module_name,
                              void *opaque) {
  JSValue result;
  char *code_cstring;
  str_t err;
  JSModuleDef *m;
  js_t* js = opaque;

  str_t code;
  str_init(js->allocator, &code, to_slice(""));

  kv_get(js->kv, str_cstring_to_slice(module_name, strlen(module_name)), &code);

  code_cstring = js->allocator.malloc(code.slice.len + 1);

  str_to_cstring(&code, code_cstring);

  result = JS_Eval(js->context, code_cstring, code.slice.len, module_name,
                   JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

  if (JS_IsException(result)) {
    JSValue exception = JS_GetException(ctx);
    const char *error = JS_ToCString(ctx, exception);
    fprintf(stderr, "JavaScript exception: %s\n", error);
    str_init(js->allocator, &err,
                    str_cstring_to_slice(error, strlen(error)));

    JS_FreeCString(ctx, error);
    JS_FreeValue(ctx, exception);
    return NULL;
  }
  m = JS_VALUE_GET_PTR(result);
  JS_FreeValue(ctx, result);

  return m;
}

js_t* js_new(allocator_t allocator, kv_i kv) {
  js_t* self;
  self = allocator.malloc(sizeof *self);
  self->allocator = allocator;
  self->runtime = JS_NewRuntime();
  JS_SetModuleLoaderFunc(self->runtime, NULL, kv_module_loader, self);
  self->context = JS_NewContext(self->runtime);
  JS_SetContextOpaque(self->context, self);
  self->kv = kv;
  JS_SetDumpFlags(self->runtime, JS_DUMP_MODULE_RESOLVE);

  return self;
}

void js_delete(js_t* self) {
  JS_FreeContext(self->context);
  JS_FreeRuntime(self->runtime);
  self->allocator.free(self);
}


void js_run(js_options_t options) {
    
}
