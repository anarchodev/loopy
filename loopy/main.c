#include <stddef.h>

#include "allocator/allocator.h"
#include "internal.h"
#include "serve/serve.h"

int main(int argc, char **argv) {
  srv_options_t options;
  srv_i srv;

  options = parse_args(argc, argv);

  srv = srv_new(allocator_default(), options);

  srv_run(srv);

  srv_delete(srv);

  return 0;
}

