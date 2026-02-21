#include "serve/serve_internal.h"

void srv_response_status_set(srv_response_t *res, int status) {
  res->status = status;
}

srv_response_t *srv_response_new(allocator_t allocator) {
  srv_response_t *res = allocator.malloc(sizeof(*res));
  if (res == NULL)
    return res;
  str_fixed_init(&res->body, str_cstring_to_slice(res->body_buffer, 0),
                 sizeof(res->body_buffer));

  str_fixed_init(&res->headers, str_cstring_to_slice(res->headers_buffer, 0),
                 sizeof res->headers_buffer);

  str_fixed_init(&res->status_line,
                 str_cstring_to_slice(res->status_line_buffer, 0),
                 sizeof res->status_line_buffer);
  res->allocator = allocator;
  return res;
}

int srv_response_headers_append(srv_response_t *res, str_slice_t header) {
  int n;
  n = str_fixed_append(&res->headers, header);
  if (n < 0) {
    return n;
  }

  return str_fixed_append(&res->headers, to_slice("\r\n"));
}

int srv_response_body_append(srv_response_t *self, str_slice_t body) {
  return str_fixed_append(&self->body, body);
}
