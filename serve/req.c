
#include "serve/serve.h"
#include "serve/serve_internal.h"

srv_request_t *srv_request_new(allocator_t allocator, srv_request_cb cb) {
  srv_request_t *request = allocator.malloc(sizeof(*request));
  if (request == NULL)
    return request;
  request->headers_count =
      sizeof(request->headers) / sizeof(request->headers[0]);
  request->allocator = allocator;
  request->request_cb = cb;
  request->current_buffer.ptr = request->buffer;
  request->current_buffer.len = 0;

  return request;
}

void srv_request_delete( srv_request_t*self) {
    self->allocator.free(self);
}
