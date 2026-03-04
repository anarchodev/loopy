#include "serve/serve_internal.h"

void srv_response_status_set(srv_response_t *res, int status) {
  res->status = status;
}

srv_response_t *srv_response_new(allocator_t allocator) {
  srv_response_t *res = allocator.malloc(sizeof(*res));
  if (res == NULL)
    return res;
  res->body = str_fixed_init(str_cstring_to_slice(res->body_buffer, 0),
                             sizeof(res->body_buffer));

  res->headers = str_fixed_init(str_cstring_to_slice(res->headers_buffer, 0),
                                sizeof res->headers_buffer);

  res->status_line =
      str_fixed_init(str_cstring_to_slice(res->status_line_buffer, 0),
                     sizeof res->status_line_buffer);
  res->allocator = allocator;
  res->status = 200;
  return res;
}

int srv_response_headers_append(srv_response_t *res, str_slice_t header) {
  if (header.len + 2 > res->headers.capacity - res->headers.s.len)
    return -1;
  str_fixed_append(&res->headers, header);
  return str_fixed_append(&res->headers, to_slice("\r\n"));
}

int srv_response_body_append(srv_response_t *self, str_slice_t body) {
  return str_fixed_append(&self->body, body);
}

void srv_response_delete(srv_response_t *self) {
  self->allocator.free(self);
}
