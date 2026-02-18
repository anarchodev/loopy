#ifndef serve_internal_h_INCLUDED
#define serve_internal_h_INCLUDED
#include "serve.h"
#include <string_view/string_view.h>

#include <uv.h>
#include <picohttpparser.h>

typedef struct srv_s {
  allocator_t allocator;
  uv_loop_t *loop;
  uv_tcp_t listen_socket;
  struct sockaddr_in addr;
  srv_options_t options;
  void (*request_cb)(srv_request_i, srv_response_i);
} srv_t;

typedef struct srv_request_s {
  allocator_t allocator;
  struct phr_header headers[100];
  size_t headers_count;
  char buffer[512];
  sv_t current_buffer;
  sv_t buffer_previous;
  sv_t path;
  sv_t method;
  int minor_version;
  void (*request_cb)(srv_request_i, srv_response_i);
} srv_request_t;

typedef struct srv_response_s {
  allocator_t allocator;
  int status;
  char buf[512];
  sv_t buffer;
} srv_response_t;

srv_request_t *srv_request_new(allocator_t allocator, srv_request_cb cb);
void srv_request_delete(srv_request_t *);

srv_response_t *srv_response_new(allocator_t allocator);
void srv_response_delete(srv_response_t *);

void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
void write_cb(uv_write_t *, int);
void connection_cb(uv_stream_t *stream, int status);

void alloc_buf(uv_handle_t *stream, size_t suggested_size, uv_buf_t *buf);
#endif // serve_internal_h_INCLUDED
