#include "serve/serve_internal.h"
#include "serve/test.h"

void srv_request_set_method(srv_request_i self, str_slice_t method) {
  self->method = method;
}
