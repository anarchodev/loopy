#include "internal.h"
#include <assert.h>

int phr_parse_request_return;

char __uv_buf[512];

int phr_parse_request(const char *, size_t, const char **, size_t *,
                      const char **, size_t *, int *, struct phr_header *,
                      size_t *, size_t) {
  return phr_parse_request_return;
}

int uv_accept(uv_stream_t *, uv_stream_t *) { return 0; }
char *uv_strerror_r(int, char *, size_t) { return ""; }
int uv_read_stop(uv_stream_t *) { return 0; }
int uv_read_start(uv_stream_t *, uv_alloc_cb, uv_read_cb) { return 0; }
void uv_close(uv_handle_t *, uv_close_cb) {}
int uv_tcp_init(uv_loop_t *, uv_tcp_t *) { return 0; }

int main() {
  uv_buf_t buf;
  uv_stream_t stream;
  static loopy_request_t request;

  buf.base = malloc(2);
  buf.base[0] = 'a';
  buf.base[1] = 'b';

  buf.len = 2;

  stream.data = &request;

  phr_parse_request_return = -2;

  read_cb(&stream, 1, &buf);

  assert(request.buf[0] == 'a');
  assert(request.buf[1] == 0);

  phr_parse_request_return = 1;

  buf.base = malloc(2);
  buf.base[0] = 'b';
  buf.base[1] = 'c';

  read_cb(&stream, 1, &buf);

  assert(request.buf[0] == 'a');
  assert(request.buf[1] == 'b');
  assert(request.buf[2] == 0);
  assert(request.buf_len == 2);

  return 0;
}
