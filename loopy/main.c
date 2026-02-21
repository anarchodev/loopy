#include <allocator/allocator.h>
#include <serve/serve.h>
#include <log/log.h>

#include "internal.h"

void loopy_on_request(srv_request_i request, srv_response_i response) {
    (void)(request);
    log_info("loopy_on_request called");
    srv_response_status_set(response, 200);
    srv_response_body_append(response, to_slice("OK"));
}

int main(int argc, char **argv) {
  srv_options_t options;
  srv_i srv;

  options = loopy_parse_args(argc, argv);

  options.request_cb = loopy_on_request;

  srv = srv_new(allocator_default(), options);

  srv_run(srv);

  srv_delete(srv);

  return 0;
}

