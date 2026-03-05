#include "js.h"
#include "allocator/allocator.h"
#include "base64/base64.h"
#include "js/js_internal.h"
#include "kv/kv.h"
#include "quickjs.h"
#include "serve/serve.h"
#include "str/str.h"
#include <assert.h>

static JSModuleDef *try_load_bytecode(js_t *js, JSContext *ctx,
                                      str_slice_t bc_key) {
  str_t bc_b64;
  JSModuleDef *m = NULL;

  str_init(js->allocator, &bc_b64, to_slice(""));
  kv_get(js->kv, bc_key, &bc_b64);

  if (bc_b64.slice.len > 0) {
    size_t buf_len = (bc_b64.slice.len / 4) * 3 + 3;
    uint8_t *buf = js->allocator.malloc(buf_len);
    int decoded = base64_decode(bc_b64.slice.ptr, bc_b64.slice.len, buf);
    if (decoded > 0) {
      JSValue result = JS_ReadObject(ctx, buf, (size_t)decoded, JS_READ_OBJ_BYTECODE);
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
  uint8_t *bc_buf = JS_WriteObject(ctx, &bc_size, result, JS_WRITE_OBJ_BYTECODE);
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
  str_slice_t module_name_slice = str_cstring_to_slice(module_name, strlen(module_name));

  str_t bc_key;
  str_init(js->allocator, &bc_key, js->kv_prefix.slice);
  str_append(js->allocator, &bc_key, to_slice(".bytecode."));
  str_append(js->allocator, &bc_key, module_name_slice);

  m = try_load_bytecode(js, ctx, bc_key.slice);
  if (m) {
    str_deinit(js->allocator, &bc_key);
    return m;
  }

  str_t key;
  str_t code;
  str_init(js->allocator, &key, js->kv_prefix.slice);
  str_append(js->allocator, &key, to_slice(".files."));
  str_append(js->allocator, &key, module_name_slice);
  str_init(js->allocator, &code, to_slice(""));
  kv_get(js->kv, key.slice, &code);
  str_deinit(js->allocator, &key);

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
  self->kv_prefix = options.kv_prefix;
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

int js_run(allocator_t allocator, js_options_t options) {
  js_t *js;

  int is_get;
  int is_post;
  str_slice_t method;

  method = srv_request_get_method(options.request);

  is_get = is_post = 0;

  is_get = str_slice_eq(method, to_slice("GET"));
  if (!is_get) {
    is_post = str_slice_eq(method, to_slice("POST"));
  }
  if (!is_post && !is_get) {
    return -1;
  }
  js = js_new(allocator, options);


  return 0;
}
