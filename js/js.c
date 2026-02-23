#include "js.h"
#include "allocator/allocator.h"
#include "js/js_internal.h"
#include "quickjs.h"
#include "str/str.h"
#include <assert.h>

JSModuleDef *kv_module_loader(JSContext *ctx, const char *module_name,
                              void *opaque) {
  (void)(opaque);
  (void)(ctx);
  (void)(module_name);
  assert(false);
}

js_i js_new(allocator_t allocator, kv_i kv) {
  js_i self;
  self = allocator.malloc(sizeof *self);
  self->allocator = allocator;
  self->runtime = JS_NewRuntime();
  self->context = JS_NewContext(self->runtime);
  JS_SetContextOpaque(self->context, self);
  self->kv = kv;

  JS_SetModuleLoaderFunc(self->runtime, NULL, kv_module_loader, self);
  return self;
}

void js_delete(js_i self) {
  JS_FreeContext(self->context);
  JS_FreeRuntime(self->runtime);
  self->allocator.free(self);
}

void js_set_opaque(js_i self, void *data) { self->opaque = data; }

void *js_get_opaque(js_i self) { return self->opaque; }

void js_eval(js_i self, str_slice_t func_path, str_slice_t args,
             js_eval_cb cb) {
  JSValue result;
  JSValue jsjson;

  const char code[] = "export function hello_world(name) {\n"
  "  return `hello ${}`;\n"
  "}\n";
  const char *json;
  str_dynamic_t err;
  str_dynamic_t djson;

  (void)(func_path);
  (void)(args);

  result = JS_Eval(self->context, code, sizeof code, "index.js", JS_EVAL_TYPE_MODULE);

  if (JS_IsException(result)) {
    JSValue exception = JS_GetException(self->context);
    const char *error = JS_ToCString(self->context, exception);
    fprintf(stderr, "JavaScript exception: %s\n", error);
    str_dynamic_new(self->allocator, &err,
                    str_cstring_to_slice(error, strlen(error)));
    cb(self, &err, -1);
    JS_FreeCString(self->context, error);
    JS_FreeValue(self->context, exception);
    return;
  }

  jsjson = JS_JSONStringify(self->context, result, JS_UNDEFINED, JS_UNDEFINED);

  json = JS_ToCString(self->context, jsjson);

  str_dynamic_new(self->allocator, &djson,
                  str_cstring_to_slice(json, strlen(json)));

  cb(self, &djson, 0);

  JS_FreeCString(self->context, json);
  JS_FreeValue(self->context, jsjson);
  JS_FreeValue(self->context, result);
}
