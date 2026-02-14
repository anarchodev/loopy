#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <getopt.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <quickjs.h>
#pragma GCC diagnostic pop
#include <sqlite3.h>
#include <uv.h>

const char *short_opts = ":Hpb:";
char __uv_buf[512];
char __host[512] = "127.0.0.1";
int __port = 7070;
int __backlog = 128;

struct option long_opts[] = {{"host", required_argument, NULL, 'H'},
                             {"port", required_argument, NULL, 'p'},
                             {"backlog", required_argument, NULL, 'b'},
                             {NULL, 0, NULL, 0}};

static inline void uv_ensure(int err) __attribute__((always_inline));

static inline void uv_ensure(int err) {
  if (err >= 0) {
    return;
  }
  printf("%s", uv_strerror_r(err, __uv_buf, sizeof(__uv_buf)));
  exit(err);
}

void parse_args(int argc, char **argv) {
  while (1) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);
    if (opt == -1)
      break;

    switch (opt) {
    case 'H': /* ... */
      const int len = sizeof(__host) / sizeof(__host[0]);
      strncpy(__host, optarg, len);
      __host[len - 1] = 0;
      break;
    case 'p': /* ... */
      __port = atoi(optarg);
      break;
    case 'b': /* ... */
      __backlog = atoi(optarg);
      break;
    case '?': /* ... unknown option */
    case ':': /* ... option missing required argument */
    default:
      /* ... unexpected; here for completeness */
    }
  }
}

void connection_cb([[maybe_unused]] uv_stream_t *server, [[maybe_unused]] int status) {
    
    
}

int main(int argc, char **argv) {
  parse_args(argc, argv);

  uv_loop_t *loop = uv_default_loop();

  uv_tcp_t server;
  uv_ensure(uv_tcp_init(loop, &server));

  struct sockaddr_in addr;
  uv_ensure(uv_ip4_addr(__host, __port, &addr));

  uv_ensure(uv_tcp_bind(&server, (struct sockaddr*)&addr, 0));

  uv_ensure(uv_listen((uv_stream_t*)&server, __backlog, connection_cb));

  uv_run(loop, UV_RUN_DEFAULT);

  uv_loop_close(loop);

  return 0;
}

