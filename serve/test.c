#include "allocator/allocator.h"
#include "serve/serve.h"
#include "serve/serve_internal.h"
#include "str/str.h"
#include <assert.h>
#include <string.h>

/* --- response tests --- */

static void test_response_new_delete(void) {
  allocator_t a = allocator_default();
  srv_response_i res = srv_response_new(a);
  assert(res != NULL);
  srv_response_delete(res);
}

static void test_response_status_set(void) {
  allocator_t a = allocator_default();
  srv_response_t *res = srv_response_new(a);
  srv_response_status_set(res, 404);
  assert(res->status == 404);
  srv_response_delete(res);
}

static void test_response_body_append(void) {
  allocator_t a = allocator_default();
  srv_response_t *res = srv_response_new(a);
  assert(srv_response_body_append(res, to_slice("hello")) == 0);
  assert(res->body.s.len == 5);
  assert(memcmp(res->body.s.ptr, "hello", 5) == 0);
  srv_response_delete(res);
}

static void test_response_body_append_overflow(void) {
  allocator_t a = allocator_default();
  char big[4097];
  srv_response_t *res = srv_response_new(a);
  memset(big, 'x', sizeof big);
  assert(srv_response_body_append(
             res, str_cstring_to_slice(big, sizeof big)) == -1);
  assert(res->body.s.len == 0);
  srv_response_delete(res);
}

static void test_response_headers_append(void) {
  allocator_t a = allocator_default();
  srv_response_t *res = srv_response_new(a);
  assert(srv_response_headers_append(
             res, to_slice("Content-Type: text/plain")) == 0);
  /* headers_append appends \r\n automatically */
  assert(res->headers.s.len == 26);
  assert(memcmp(res->headers.s.ptr, "Content-Type: text/plain\r\n", 26) == 0);
  srv_response_delete(res);
}

/* --- request tests --- */

static void test_request_new_delete(void) {
  allocator_t a = allocator_default();
  srv_request_i req = srv_request_new(a, NULL);
  assert(req != NULL);
  srv_request_delete(req);
}

static void test_request_get_method(void) {
  allocator_t a = allocator_default();
  srv_request_t *req = srv_request_new(a, NULL);
  req->method = to_slice("GET");
  assert(str_slice_eq(srv_request_get_method(req), to_slice("GET")));
  srv_request_delete(req);
}

static void test_request_get_body(void) {
  allocator_t a = allocator_default();
  srv_request_t *req = srv_request_new(a, NULL);
  req->body = to_slice("hello");
  assert(str_slice_eq(srv_request_get_body(req), to_slice("hello")));
  srv_request_delete(req);
}

int main(void) {
  test_response_new_delete();
  test_response_status_set();
  test_response_body_append();
  test_response_body_append_overflow();
  test_response_headers_append();

  test_request_new_delete();
  test_request_get_method();
  test_request_get_body();

  return 0;
}
