#ifndef serve_internal_h_INCLUDED
#define serve_internal_h_INCLUDED
#include "serve.h"

#include <picohttpparser.h>
#include <uv.h>

typedef struct srv_s {
  allocator_t allocator;
  uv_loop_t *loop;
  uv_tcp_t listen_socket;
  struct sockaddr_in addr;
  int port;
  int backlog;
  const char *host;
  void (*request_cb)(srv_request_i, srv_response_i);
} srv_t;

typedef struct srv_request_s {
  allocator_t allocator;
  struct phr_header headers[100];
  size_t headers_count;
  char buffer[4096];
  str_slice_t current_buffer;
  str_slice_t buffer_previous;
  str_slice_t path;
  str_slice_t method;
  int minor_version;
  void (*request_cb)(srv_request_i, srv_response_i);
} srv_request_t;

typedef struct srv_response_s {
  allocator_t allocator;
  int status;
  char headers_buffer[1024];
  str_fixed_t headers;
  char status_line_buffer[512];
  str_fixed_t status_line;
  char body_buffer[4096];
  str_fixed_t body;
}srv_response_t;

srv_request_t *srv_request_new(allocator_t allocator, srv_request_cb cb);
void srv_request_delete(srv_request_t *);

srv_response_t *srv_response_new(allocator_t allocator);
void srv_response_delete(srv_response_t *);

void srv_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
void srv_write_cb(uv_write_t *, int);
void srv_connection_cb(uv_stream_t *stream, int status);

void alloc_buf(uv_handle_t *stream, size_t suggested_size, uv_buf_t *buf);

str_slice_t srv_response_status_get(srv_response_t*);
str_slice_t srv_response_headers_get(srv_response_t*);
str_slice_t srv_response_body_get(srv_response_t*);

#endif // serve_internal_h_INCLUDED
