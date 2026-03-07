#include "allocator/allocator.h"
#include "js/js.h"
#include "kv/kv.h"
#include "serve/serve.h"
#include "serve/serve_internal.h"
#include "str/str.h"
#include <assert.h>
#include <string.h>

static const char *INDEX_MJS =
    "export function get() { return 'hello'; }\n"
    "export function post() { return { ok: true }; }\n";

static js_i make_js(allocator_t a, kv_i kv,
                    srv_request_t *req, srv_response_t *res) {
  js_options_t opts;
  opts.kv       = kv;
  opts.request  = req;
  opts.response = res;
  return js_new(a, opts);
}

static void test_get_returns_string(void) {
  allocator_t a = allocator_default();
  kv_i kv;
  srv_request_t *req;
  srv_response_t *res;
  js_i js;

  kv = kv_new(a);
  kv_set(kv, to_slice("_js.f.index.mjs"),
         str_cstring_to_slice(INDEX_MJS, strlen(INDEX_MJS)));

  req = srv_request_new(a, NULL);
  req->method = to_slice("GET");
  res = srv_response_new(a);

  js = make_js(a, kv, req, res);
  assert(js_run(js, to_slice("index.mjs")) == 0);
  assert(str_slice_eq(res->body.s, to_slice("hello")));

  js_delete(js);
  srv_request_delete(req);
  srv_response_delete(res);
  kv_delete(kv);
}

static void test_post_returns_json(void) {
  allocator_t a = allocator_default();
  kv_i kv;
  srv_request_t *req;
  srv_response_t *res;
  js_i js;

  kv = kv_new(a);
  kv_set(kv, to_slice("_js.f.index.mjs"),
         str_cstring_to_slice(INDEX_MJS, strlen(INDEX_MJS)));

  req = srv_request_new(a, NULL);
  req->method = to_slice("POST");
  res = srv_response_new(a);

  js = make_js(a, kv, req, res);
  assert(js_run(js, to_slice("index.mjs")) == 0);
  assert(str_slice_eq(res->body.s, to_slice("{\"ok\":true}")));

  js_delete(js);
  srv_request_delete(req);
  srv_response_delete(res);
  kv_delete(kv);
}

static void test_unknown_method_returns_error(void) {
  allocator_t a = allocator_default();
  kv_i kv;
  srv_request_t *req;
  srv_response_t *res;
  js_i js;

  kv = kv_new(a);
  kv_set(kv, to_slice("_js.f.index.mjs"),
         str_cstring_to_slice(INDEX_MJS, strlen(INDEX_MJS)));

  req = srv_request_new(a, NULL);
  req->method = to_slice("PATCH");
  res = srv_response_new(a);

  js = make_js(a, kv, req, res);
  assert(js_run(js, to_slice("index.mjs")) == -1);

  js_delete(js);
  srv_request_delete(req);
  srv_response_delete(res);
  kv_delete(kv);
}

int main(void) {
  test_get_returns_string();
  test_post_returns_json();
  test_unknown_method_returns_error();
  return 0;
}
