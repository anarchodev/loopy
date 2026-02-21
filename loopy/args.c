#include "internal.h"

#include <getopt.h>
#include <serve/serve.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

const char *short_opts = ":H:p:b:";

struct option long_opts[] = {{"host", required_argument, NULL, 'H'},
                             {"port", required_argument, NULL, 'p'},
                             {"backlog", required_argument, NULL, 'b'},
                             {NULL, 0, NULL, 0}};

srv_options_t loopy_parse_args(int argc, char **argv) {
  srv_options_t options;
  options.backlog = 128;
  options.host = "127.0.0.1";
  options.port = 7070;

  while (1) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);
    if (opt == -1)
      break;

    switch (opt) {
    case 'H':
      options.host = optarg;
      break;
    case 'p':
      options.port = atoi(optarg);
      break;
    case 'b':
      options.backlog = atoi(optarg);
      break;
    case '?': /* ... unknown option */
    case ':': /* ... option missing required argument */
    default:
      break;/* ... unexpected; here for completeness */
    }
  }
  return options;
}

