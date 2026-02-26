#include "js.h"
#include "allocator/allocator.h"
#include "js/js_internal.h"
#include "kv/kv.h"
#include "quickjs.h"
#include "serve/serve.h"
#include "str/str.h"
#include <assert.h>

JSModuleDef *kv_module_loader(JSContext *ctx, const char *module_name,
                              void *opaque) {
  JSValue result;
  char *code_cstring;
  str_dynamic_t err;
  JSModuleDef *m;
  js_i js = opaque;

  str_dynamic_t code;
  str_dynamic_new(js->allocator, &code, to_slice(""));

  kv_get(js->kv, str_cstring_to_slice(module_name, strlen(module_name)), &code);

  code_cstring = js->allocator.malloc(code.slice.len + 1);

  str_dynamic_to_cstring(&code, code_cstring);

  result = JS_Eval(js->context, code_cstring, code.slice.len, module_name,
                   JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

  if (JS_IsException(result)) {
    JSValue exception = JS_GetException(ctx);
    const char *error = JS_ToCString(ctx, exception);
    fprintf(stderr, "JavaScript exception: %s\n", error);
    str_dynamic_new(js->allocator, &err,
                    str_cstring_to_slice(error, strlen(error)));

    JS_FreeCString(ctx, error);
    JS_FreeValue(ctx, exception);
    return NULL;
  }
  m = JS_VALUE_GET_PTR(result);
  JS_FreeValue(ctx, result);

  return m;
}

js_i js_new(allocator_t allocator, kv_i kv) {
  js_i self;
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

void js_delete(js_i self) {
  JS_FreeContext(self->context);
  JS_FreeRuntime(self->runtime);
  self->allocator.free(self);
}

void js_set_opaque(js_i self, void *data) { self->opaque = data; }

void *js_get_opaque(js_i self) { return self->opaque; }

void js_eval(js_i self, str_slice_t func_path, str_slice_t args,
             srv_request_i req, srv_response_i res) {
  JSValue result;
  JSValue presult;
  JSValue ppresult;
  JSValue jsjson;
  JSValue namespace;
  JSValue blah;

  const char code[] = "import { hello_world } from './index.js'\n"
                      "export const blah = hello_world('test');\n";
  const char *json;
  str_dynamic_t err;
  str_dynamic_t djson;

  (void)(func_path);
  (void)(args);

  result = JS_Eval(self->context, code, sizeof code - 1, "<eval>",
                   JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

  if (JS_IsException(result)) {
    JSValue exception = JS_GetException(self->context);
    const char *error = JS_ToCString(self->context, exception);
    fprintf(stderr, "JavaScript exception: %s\n", error);
    str_dynamic_new(self->allocator, &err,
                    str_cstring_to_slice(error, strlen(error)));
    JS_FreeCString(self->context, error);
    JS_FreeValue(self->context, exception);
    
    cb(self, &err, -1);
    return;
  }

  presult = JS_EvalFunction(self->context, JS_DupValue(self->context, result));
  ppresult = JS_PromiseResult(self->context, presult);

  JS_FreeValue(self->context, presult);
  JS_FreeValue(self->context, ppresult);

  namespace = JS_GetModuleNamespace(self->context, JS_VALUE_GET_PTR(result));

  blah = JS_GetPropertyStr(self->context, namespace, "blah");
  JS_FreeValue(self->context, namespace);

  jsjson = JS_JSONStringify(self->context, blah, JS_UNDEFINED, JS_UNDEFINED);

  json = JS_ToCString(self->context, jsjson);

  str_dynamic_new(self->allocator, &djson,
                  str_cstring_to_slice(json, strlen(json)));
  JS_FreeCString(self->context, json);
  JS_FreeValue(self->context, jsjson);

  JS_FreeValue(self->context, result);
  cb(self, &djson, 0);
}
