#include <allocator/allocator.h>
#include <log/log.h>
#include <serve/serve.h>

#include "internal.h"
#include "js/js.h"
#include "kv/kv.h"
#include "str/str.h"

void eval_cb(js_i self, str_dynamic_t*result, int status)  {
    srv_response_i res;

    (void)(status);

    res = js_get_opaque(self) ;

    srv_response_body_append(res, result->slice);
    str_dynamic_delete(result);
    js_delete(self);
}

void loopy_on_request(srv_request_i request, srv_response_i response) {
  js_i js;
  kv_i kv;
  (void)(request);
  log_info("loopy_on_request called");

  srv_response_status_set(response, 200);

  kv = kv_new(allocator_default());

  kv_set(kv, to_slice("index.js"), to_slice("export function hello_world(name) {\n"
  "  return `hello ${name}`;\n"
  "}\n"));

  js = js_new(allocator_default(), kv);

  js_set_opaque(js, response);

  js_eval(js, to_slice("/index.js"), to_slice("\"mike\""), request, response); 
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

