
#include "serve/serve.h"
#include "serve/serve_internal.h"
#include "str/str.h"
#include <assert.h>
#include <string.h>

srv_request_t *srv_request_new(allocator_t allocator, srv_request_cb cb) {
  srv_request_t *self = allocator.malloc(sizeof(*self));
  if (self == NULL)
    return self;
  self->headers_count = sizeof(self->headers) / sizeof(self->headers[0]);
  self->allocator = allocator;
  self->request_cb = cb;
  self->headers_parsed = 0;
  self->expected_body_len = 0;
  self->body = str_slice(NULL, 0);
  self->buffer_previous_len = 0;
  self->buffer =
      str_fixed_init(str_slice(self->raw_buffer, 0), sizeof self->raw_buffer);

  return self;
}

void srv_request_delete(srv_request_t *self) { self->allocator.free(self); }

str_slice_t srv_request_get_method(srv_request_i self) { return self->method; }

str_slice_t srv_request_get_body(srv_request_i self) { return self->body; }

str_slice_t srv_request_get_buffer(srv_request_i self) {
  return self->buffer.s;
}

void srv_request_reset(srv_request_i self) {
  size_t buf_len = str_fixed_len(&self->buffer);
  size_t body_end = (size_t)(self->body.ptr + self->body.len - self->raw_buffer);
  size_t extra = buf_len - body_end;

  assert(body_end <= buf_len);

  memmove(self->raw_buffer, self->raw_buffer + body_end, extra);

  self->buffer =
      str_fixed_init(str_slice(self->raw_buffer, extra), sizeof self->raw_buffer);

  self->headers_parsed = 0;
  self->expected_body_len = 0;
  self->body = str_slice(NULL, 0);
  self->method = str_slice(NULL, 0);
  self->path = str_slice(NULL, 0);
  self->buffer_previous_len = 0;
  memset(self->headers, 0, sizeof self->headers);
  self->headers_count = sizeof(self->headers) / sizeof(self->headers[0]);
}
