#include "js.h"
#include "allocator/allocator.h"
#include "base64/base64.h"
#include "js/js_internal.h"
#include "kv/kv.h"
#include "quickjs.h"
#include "serve/serve.h"
#include "str/str.h"
#include <assert.h>
#include <string.h>

static JSModuleDef *try_load_bytecode(js_t *js, JSContext *ctx,
                                      str_slice_t bc_key) {
  str_t bc_b64;
  JSModuleDef *m = NULL;

  str_init(js->allocator, &bc_b64, to_slice(""));
  kv_get(js->kv, bc_key, &bc_b64);

  if (bc_b64.slice.len > 0) {
    size_t buf_len = (bc_b64.slice.len / 4) * 3 + 3;
    unsigned char *buf = js->allocator.malloc(buf_len);
    int decoded = base64_decode(bc_b64.slice.ptr, bc_b64.slice.len, buf);
    if (decoded > 0) {
      JSValue result =
          JS_ReadObject(ctx, buf, (size_t)decoded, JS_READ_OBJ_BYTECODE);
      js->allocator.free(buf);
      if (!JS_IsException(result)) {
        m = JS_VALUE_GET_PTR(result);
        JS_FreeValue(ctx, result);
      } else {
        JS_FreeValue(ctx, result);
      }
    } else {
      js->allocator.free(buf);
    }
  }

  str_deinit(js->allocator, &bc_b64);
  return m;
}

static void store_bytecode(js_t *js, JSContext *ctx, JSValue result,
                           str_slice_t bc_key) {
  size_t bc_size;
  unsigned char *bc_buf =
      JS_WriteObject(ctx, &bc_size, result, JS_WRITE_OBJ_BYTECODE);
  if (bc_buf) {
    size_t b64_len = ((bc_size + 2) / 3) * 4;
    char *b64_buf = js->allocator.malloc(b64_len);
    base64_encode(bc_buf, bc_size, b64_buf);
    js_free(ctx, bc_buf);
    kv_set(js->kv, bc_key, str_cstring_to_slice(b64_buf, b64_len));
    js->allocator.free(b64_buf);
  }
}

JSModuleDef *kv_module_loader(JSContext *ctx, const char *module_name,
                              void *opaque) {
  JSValue result;
  char *code_cstring;
  JSModuleDef *m;
  js_t *js = opaque;
  str_t bc_key;
  str_t file_key;
  str_t code;
  str_slice_t module_name_slice;

  module_name_slice = str_cstring_to_slice(module_name, strlen(module_name));

  str_init(js->allocator, &bc_key, to_slice("_js"));
  str_append(js->allocator, &bc_key, to_slice(".b."));
  str_append(js->allocator, &bc_key, module_name_slice);

  m = try_load_bytecode(js, ctx, bc_key.slice);
  if (m) {
    str_deinit(js->allocator, &bc_key);
    return m;
  }
  str_init(js->allocator, &file_key, to_slice("_js"));
  str_append(js->allocator, &file_key, to_slice(".f."));
  str_append(js->allocator, &file_key, module_name_slice);

  str_init(js->allocator, &code, to_slice(""));
  kv_get(js->kv, file_key.slice, &code);
  str_deinit(js->allocator, &file_key);

  code_cstring = js->allocator.malloc(code.slice.len + 1);
  str_to_cstring(&code, code_cstring);
  result = JS_Eval(js->context, code_cstring, code.slice.len, module_name,
                   JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
  js->allocator.free(code_cstring);
  str_deinit(js->allocator, &code);

  if (JS_IsException(result)) {
    JSValue exception = JS_GetException(ctx);
    const char *error = JS_ToCString(ctx, exception);
    fprintf(stderr, "JavaScript exception: %s\n", error);
    JS_FreeCString(ctx, error);
    JS_FreeValue(ctx, exception);
    str_deinit(js->allocator, &bc_key);
    return NULL;
  }

  store_bytecode(js, ctx, result, bc_key.slice);
  str_deinit(js->allocator, &bc_key);

  m = JS_VALUE_GET_PTR(result);
  JS_FreeValue(ctx, result);
  return m;
}

js_t *js_new(allocator_t allocator, js_options_t options) {
  js_t *self;
  self = allocator.malloc(sizeof *self);
  self->allocator = allocator;
  self->runtime = JS_NewRuntime();
  JS_SetModuleLoaderFunc(self->runtime, NULL, kv_module_loader, self);
  self->context = JS_NewContext(self->runtime);
  JS_SetContextOpaque(self->context, self);
  self->kv = options.kv;
  self->request = options.request;
  self->response = options.response;
#ifndef NDEBUG
  JS_SetDumpFlags(self->runtime, JS_DUMP_MODULE_RESOLVE);
#endif

  return self;
}

void js_delete(js_t *self) {
  JS_FreeContext(self->context);
  JS_FreeRuntime(self->runtime);
  self->allocator.free(self);
}

int js_run(js_t *self, str_slice_t module) {
  JSContext *ctx = self->context;
  JSValue fun, eval_result, ns, fn, ret, json_val;
  JSContext *ctx1;
  JSModuleDef *m;
  str_slice_t method;
  const char *fn_name;
  const char *body_cstr;
  size_t body_len;
  str_t file_key, code;
  char *code_cstr, *name_cstr;

  str_init(self->allocator, &file_key, to_slice("_js.f."));
  str_append(self->allocator, &file_key, module);
  str_init(self->allocator, &code, to_slice(""));
  kv_get(self->kv, file_key.slice, &code);
  str_deinit(self->allocator, &file_key);

  code_cstr = self->allocator.malloc(code.slice.len + 1);
  str_to_cstring(&code, code_cstr);

  name_cstr = self->allocator.malloc(module.len + 1);
  memcpy(name_cstr, module.ptr, module.len);
  name_cstr[module.len] = '\0';

  fun = JS_Eval(ctx, code_cstr, code.slice.len, name_cstr,
                JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
  self->allocator.free(code_cstr);
  self->allocator.free(name_cstr);
  str_deinit(self->allocator, &code);

  if (JS_IsException(fun))
    return -1;

  m = JS_VALUE_GET_PTR(fun);
  eval_result = JS_EvalFunction(ctx, fun);
  if (JS_IsException(eval_result)) {
    JS_FreeValue(ctx, eval_result);
    return -1;
  }
  JS_FreeValue(ctx, eval_result);

  while (JS_ExecutePendingJob(self->runtime, &ctx1) > 0) {}

  method = srv_request_get_method(self->request);
  if      (str_slice_eq(method, to_slice("GET")))     fn_name = "get";
  else if (str_slice_eq(method, to_slice("POST")))    fn_name = "post";
  else if (str_slice_eq(method, to_slice("PUT")))     fn_name = "put";
  else if (str_slice_eq(method, to_slice("DELETE")))  fn_name = "destroy";
  else if (str_slice_eq(method, to_slice("OPTIONS"))) fn_name = "options";
  else return -1;

  ns = JS_GetModuleNamespace(ctx, m);
  fn = JS_GetPropertyStr(ctx, ns, fn_name);
  JS_FreeValue(ctx, ns);

  if (!JS_IsFunction(ctx, fn)) {
    JS_FreeValue(ctx, fn);
    return -1;
  }

  ret = JS_Call(ctx, fn, JS_UNDEFINED, 0, NULL);
  JS_FreeValue(ctx, fn);

  if (JS_IsException(ret)) {
    JS_FreeValue(ctx, ret);
    return -1;
  }

  if (JS_IsString(ret)) {
    body_cstr = JS_ToCStringLen(ctx, &body_len, ret);
    srv_response_body_append(self->response,
                             str_cstring_to_slice(body_cstr, body_len));
    JS_FreeCString(ctx, body_cstr);
  } else {
    json_val = JS_JSONStringify(ctx, ret, JS_UNDEFINED, JS_UNDEFINED);
    if (!JS_IsException(json_val) && !JS_IsUndefined(json_val)) {
      body_cstr = JS_ToCStringLen(ctx, &body_len, json_val);
      srv_response_body_append(self->response,
                               str_cstring_to_slice(body_cstr, body_len));
      JS_FreeCString(ctx, body_cstr);
    }
    JS_FreeValue(ctx, json_val);
  }

  JS_FreeValue(ctx, ret);
  return 0;
}

