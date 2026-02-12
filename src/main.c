#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

void on_timer(uv_timer_t *req) {
  printf("hello");
  fflush(stdout);
  uv_timer_stop(req);
}

int main() {
  // start http server
  // start sqlite
  // add a route
  // for route run a javascript program that increments variable and returns it

  uv_timer_t timer_req;
  uv_timer_init(uv_default_loop(), &timer_req);

  uv_timer_start(&timer_req, on_timer, 1000, 2000);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  uv_loop_close(uv_default_loop());

  return 0;
}

