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

static JSValue try_load_bytecode(js_t *js, JSContext *ctx,
                                  str_slice_t bc_key) {
  str_t bc_b64;
  JSValue result = JS_UNDEFINED;

  str_init(js->allocator, &bc_b64, to_slice(""));
  kv_get(js->kv, bc_key, &bc_b64);

  if (bc_b64.slice.len > 0) {
    size_t buf_len = (bc_b64.slice.len / 4) * 3 + 3;
    unsigned char *buf = js->allocator.malloc(buf_len);
    int decoded = base64_decode(bc_b64.slice.ptr, bc_b64.slice.len, buf);
    if (decoded > 0) {
      result = JS_ReadObject(ctx, buf, (size_t)decoded, JS_READ_OBJ_BYTECODE);
      js->allocator.free(buf);
      if (JS_IsException(result)) {
        JS_FreeValue(ctx, result);
        result = JS_UNDEFINED;
      }
    } else {
      js->allocator.free(buf);
    }
  }

  str_deinit(js->allocator, &bc_b64);
  return result;
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

static JSValue load_module(js_t *js, JSContext *ctx, const char *module_name) {
  str_slice_t module_name_slice;
  str_t bc_key;
  str_t file_key;
  str_t code;
  char *code_cstring;
  JSValue result;

  module_name_slice = str_cstring_to_slice(module_name, strlen(module_name));

  str_init(js->allocator, &bc_key, to_slice("_js.b."));
  str_append(js->allocator, &bc_key, module_name_slice);

  result = try_load_bytecode(js, ctx, bc_key.slice);
  if (!JS_IsUndefined(result)) {
    str_deinit(js->allocator, &bc_key);
    return result;
  }

  str_init(js->allocator, &file_key, to_slice("_js.f."));
  str_append(js->allocator, &file_key, module_name_slice);

  str_init(js->allocator, &code, to_slice(""));
  kv_get(js->kv, file_key.slice, &code);
  str_deinit(js->allocator, &file_key);

  code_cstring = js->allocator.malloc(code.slice.len + 1);
  str_to_cstring(&code, code_cstring);
  result = JS_Eval(ctx, code_cstring, code.slice.len, module_name,
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
    return result;
  }

  store_bytecode(js, ctx, result, bc_key.slice);
  str_deinit(js->allocator, &bc_key);
  return result;
}

JSModuleDef *kv_module_loader(JSContext *ctx, const char *module_name,
                              void *opaque) {
  js_t *js = opaque;
  JSValue val = load_module(js, ctx, module_name);
  JSModuleDef *m;
  if (JS_IsException(val))
    return NULL;
  m = JS_VALUE_GET_PTR(val);
  JS_FreeValue(ctx, val);
  return m;
}

static JSValue js_kv_get_item(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
  js_t *js = JS_GetContextOpaque(ctx);
  const char *key_cstr;
  size_t key_len;
  str_t result;
  JSValue ret;

  (void)this_val;
  if (argc < 1 || !JS_IsString(argv[0]))
    return JS_UNDEFINED;

  key_cstr = JS_ToCStringLen(ctx, &key_len, argv[0]);
  str_init(js->allocator, &result, to_slice(""));
  kv_get(js->kv, str_cstring_to_slice(key_cstr, (unsigned long)key_len), &result);
  JS_FreeCString(ctx, key_cstr);

  ret = JS_NewStringLen(ctx, result.slice.ptr, result.slice.len);
  str_deinit(js->allocator, &result);
  return ret;
}

static JSValue js_kv_set_item(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
  js_t *js = JS_GetContextOpaque(ctx);
  const char *key_cstr, *val_cstr;
  size_t key_len, val_len;

  (void)this_val;
  if (argc < 2 || !JS_IsString(argv[0]) || !JS_IsString(argv[1]))
    return JS_UNDEFINED;

  key_cstr = JS_ToCStringLen(ctx, &key_len, argv[0]);
  val_cstr = JS_ToCStringLen(ctx, &val_len, argv[1]);
  kv_set(js->kv, str_cstring_to_slice(key_cstr, (unsigned long)key_len),
                 str_cstring_to_slice(val_cstr, (unsigned long)val_len));
  JS_FreeCString(ctx, val_cstr);
  JS_FreeCString(ctx, key_cstr);
  return JS_UNDEFINED;
}

static JSValue js_kv_remove_item(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
  js_t *js = JS_GetContextOpaque(ctx);
  const char *key_cstr;
  size_t key_len;

  (void)this_val;
  if (argc < 1 || !JS_IsString(argv[0]))
    return JS_UNDEFINED;

  key_cstr = JS_ToCStringLen(ctx, &key_len, argv[0]);
  kv_remove(js->kv, str_cstring_to_slice(key_cstr, (unsigned long)key_len));
  JS_FreeCString(ctx, key_cstr);
  return JS_UNDEFINED;
}

static void js_setup_kv_global(js_t *self) {
  JSContext *ctx = self->context;
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue kv_obj = JS_NewObject(ctx);

  JS_SetPropertyStr(ctx, kv_obj, "getItem",
      JS_NewCFunction(ctx, js_kv_get_item, "getItem", 1));
  JS_SetPropertyStr(ctx, kv_obj, "setItem",
      JS_NewCFunction(ctx, js_kv_set_item, "setItem", 2));
  JS_SetPropertyStr(ctx, kv_obj, "removeItem",
      JS_NewCFunction(ctx, js_kv_remove_item, "removeItem", 1));

  JS_SetPropertyStr(ctx, global, "KV", kv_obj);
  JS_FreeValue(ctx, global);
}

static void js_setup_request_global(js_t *self) {
  JSContext *ctx = self->context;
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue req_obj = JS_NewObject(ctx);
  str_slice_t method = srv_request_get_method(self->request);
  str_slice_t path   = srv_request_get_path(self->request);
  str_slice_t body   = srv_request_get_body(self->request);

  JS_SetPropertyStr(ctx, req_obj, "method",
      JS_NewStringLen(ctx, method.ptr, method.len));
  JS_SetPropertyStr(ctx, req_obj, "path",
      JS_NewStringLen(ctx, path.ptr, path.len));
  JS_SetPropertyStr(ctx, req_obj, "body",
      JS_NewStringLen(ctx, body.ptr, body.len));

  {
    size_t i;
    size_t count = srv_request_get_header_count(self->request);
    JSValue headers_obj = JS_NewObject(ctx);
    for (i = 0; i < count; i++) {
      str_slice_t name  = srv_request_get_header_name(self->request, i);
      str_slice_t value = srv_request_get_header_value(self->request, i);
      JSAtom atom = JS_NewAtomLen(ctx, name.ptr, name.len);
      JS_SetProperty(ctx, headers_obj, atom,
          JS_NewStringLen(ctx, value.ptr, value.len));
      JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, req_obj, "headers", headers_obj);
  }

  JS_SetPropertyStr(ctx, global, "REQUEST", req_obj);
  JS_FreeValue(ctx, global);
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
  js_setup_kv_global(self);
  js_setup_request_global(self);
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

int js_run(js_t *self) {
  JSContext *ctx = self->context;
  JSValue fun, eval_result, ns, fn, ret, json_val;
  JSContext *ctx1;
  JSModuleDef *m;

  str_slice_t method;
  const char *fn_name;
  const char *body_cstr;
  size_t body_len;

  fun = load_module(self, ctx, "index.mjs");
  if (JS_IsException(fun))
    return -1;

  m = JS_VALUE_GET_PTR(fun);
  eval_result = JS_EvalFunction(ctx, fun);
  if (JS_IsException(eval_result)) {
    JS_FreeValue(ctx, eval_result);
    return -1;
  }
  JS_FreeValue(ctx, eval_result);

  while (JS_ExecutePendingJob(self->runtime, &ctx1) > 0) {
  }

  method = srv_request_get_method(self->request);
  if (str_slice_eq(method, to_slice("GET")))
    fn_name = "get";
  else if (str_slice_eq(method, to_slice("POST")))
    fn_name = "post";
  else if (str_slice_eq(method, to_slice("PUT")))
    fn_name = "put";
  else if (str_slice_eq(method, to_slice("DELETE")))
    fn_name = "destroy";
  else if (str_slice_eq(method, to_slice("OPTIONS")))
    fn_name = "options";
  else
    return -1;

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

  if (JS_IsPromise(ret)) {
    while (JS_PromiseState(ctx, ret) == JS_PROMISE_PENDING &&
           JS_ExecutePendingJob(self->runtime, &ctx1) > 0) {
    }
    if (JS_PromiseState(ctx, ret) == JS_PROMISE_FULFILLED) {
      JSValue resolved = JS_PromiseResult(ctx, ret);
      JS_FreeValue(ctx, ret);
      ret = resolved;
    } else {
      JS_FreeValue(ctx, ret);
      return -1;
    }
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

