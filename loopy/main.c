#include <allocator/allocator.h>
#include <log/log.h>
#include <serve/serve.h>

#include "internal.h"
#include "js/js.h"
#include "kv/kv.h"
#include "str/str.h"

void loopy_on_request(srv_request_i request, srv_response_i response) {
  kv_i kv;
  js_options_t options;
  js_i js;
  (void)(request);
  log_info("loopy_on_request called");

  srv_response_status_set(response, 200);

  kv = kv_new(allocator_default());

  options.kv = kv;
  options.request = request;
  options.response = response;

  js = js_new(allocator_default(), options);

  js_run(js);

  js_delete(js);

  kv_delete(kv);
}

int main(int argc, char **argv) {
  loopy_options_t options;
  srv_options_t srv_options;
  srv_i srv;

  options = loopy_parse_args(argc, argv);

  if (options.reset) {
    kv_i kv = kv_new(allocator_default());
    loopy_load_mjs(allocator_default(), kv, "mjs");
    kv_delete(kv);
  }

  srv_options.host = options.host;
  srv_options.port = options.port;
  srv_options.backlog = options.backlog;
  srv_options.request_cb = loopy_on_request;

  srv = srv_new(allocator_default(), srv_options);

  srv_run(srv);

  srv_delete(srv);

  return 0;
}

