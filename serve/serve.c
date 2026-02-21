#include <allocator/allocator.h>
#include <assert.h>
#include <log/log.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

#include "serve.h"
#include "serve_internal.h"
#include "str/str.h"

srv_i srv_new(allocator_t allocator, srv_options_t options) {
  srv_t *srv = allocator.malloc(sizeof(*srv));

  srv->allocator = allocator;
  srv->request_cb = options.request_cb;
  srv->port = options.port;
  srv->host = options.host;
  srv->backlog = options.backlog;

  return srv;
}

void srv_delete(srv_i srv) { srv->allocator.free(srv); }

void srv_response_status_set(srv_response_t *res, int status) {
  res->status = status;
}

void free_buf(allocator_t alloc, const uv_buf_t *buf) {
  if (buf->len > 0)
    alloc.free(buf->base);
}

void alloc_buf(uv_handle_t *stream, size_t suggested_size, uv_buf_t *buf) {
  srv_request_t *request = stream->data;
  allocator_t alloc = request->allocator;
  buf->base = alloc.malloc(suggested_size);
  buf->len = suggested_size;
}

void srv_write_cb(uv_write_t *req, int) {
  srv_response_t *res = req->data;
  res->allocator.free(res);
}

srv_request_t *srv_request_new(allocator_t allocator, srv_request_cb cb) {
  srv_request_t *request = allocator.malloc(sizeof(*request));
  if (request == NULL)
    return request;
  request->headers_count =
      sizeof(request->headers) / sizeof(request->headers[0]);
  request->allocator = allocator;
  request->request_cb = cb;
  request->current_buffer.ptr = request->buffer;
  request->current_buffer.len = 0;

  return request;
}

srv_response_t *srv_response_new(allocator_t allocator) {
  srv_response_t *res = allocator.malloc(sizeof(*res));
  if (res == NULL)
    return res;
  str_fixed_init(&res->body, str_cstring_to_slice(res->body_buffer, 0),
                 sizeof(res->body_buffer));

  str_fixed_init(&res->headers, str_cstring_to_slice(res->headers_buffer, 0),
                 sizeof res->headers_buffer);

  str_fixed_init(&res->status_line,
                 str_cstring_to_slice(res->status_line_buffer, 0),
                 sizeof res->status_line_buffer);
  res->allocator = allocator;
  return res;
}

int srv_response_headers_append(srv_response_t *res, str_slice_t header) {
  int n;
  n = str_fixed_append(&res->headers, header);
  if (n < 0) {
    return n;
  }

  return str_fixed_append(&res->headers, to_slice("\r\n"));
}

int srv_response_body_append(srv_response_t *self, str_slice_t body) {
  return str_fixed_append(&self->body, body);
}

void srv_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  srv_request_t *request = stream->data;
  allocator_t alloc = request->allocator;
  if (nread < 0)
    goto done;
  if (nread == UV_EOF)
    goto done;

  if (nread + request->current_buffer.len > (long)sizeof(request->buffer)) {
    // to big
    goto done;
  }

  memcpy(request->buffer + request->current_buffer.len, buf->base, nread);

  request->buffer_previous.len = request->current_buffer.len;
  request->current_buffer.len = request->current_buffer.len + nread;

  int ret = phr_parse_request(
      request->buffer, request->current_buffer.len,
      (const char **)&request->method.ptr, &request->method.len,
      (const char **)&request->path.ptr, &request->path.len,
      &request->minor_version, request->headers, &request->headers_count,
      request->buffer_previous.len);

  if (ret > 0) {
    // done
    srv_response_t *response = srv_response_new(alloc);

    if (request->request_cb) {
      request->request_cb(request, response);

      uv_write_t *write = alloc.malloc(sizeof(*write));
      write->data = response;

      uv_buf_t bufs[3];

      str_fixed_append(&response->status_line, to_slice("HTTP/1.1 "));
      str_fixed_append_long(&response->status_line, response->status);
      str_fixed_append(&response->status_line, to_slice(" OK\r\n"));

      str_fixed_append(&response->headers, to_slice("Content-Length: "));
      str_fixed_append_long(&response->headers, response->body.s.len);
      str_fixed_append(&response->headers, to_slice("\r\n\r\n"));

      bufs[0] =
          uv_buf_init(response->status_line.s.ptr, response->status_line.s.len);
      bufs[1] = uv_buf_init(response->headers.s.ptr, response->headers.s.len);
      bufs[2] = uv_buf_init(response->body.s.ptr, response->body.s.len);

      uv_write(write, stream, bufs, 3, srv_write_cb);

      goto done;
    }

    goto done;

  } else if (ret == -1) {
    goto done;
  }

  free_buf(alloc, buf);

  return;

done:
  free_buf(alloc, buf);
  uv_close((uv_handle_t *)stream, NULL);
  uv_read_stop(stream);
}

void srv_connection_cb(uv_stream_t *stream, int status) {
  if (status < 0) {
    log_error("failed to connect");
    return;
  }
  srv_t *srv = stream->data;
  allocator_t alloc = srv->allocator;
  srv_request_t *request = srv_request_new(alloc, srv->request_cb);
  request->headers_count =
      sizeof(request->headers) / sizeof(request->headers[0]);
  uv_tcp_t *client = alloc.malloc(sizeof(*client));
  client->data = request;
  uv_tcp_init(srv->loop, client);
  if ((status = uv_accept(stream, (uv_stream_t *)client)) < 0)
    goto fail;

  if ((status = uv_read_start((uv_stream_t *)client, alloc_buf, srv_read_cb)) <
      0) {
    goto fail;
  }

  return;

fail:
  alloc.free(client);
}

static void close_walk_cb(uv_handle_t *handle, void *) {
  if (!uv_is_closing(handle))
    uv_close(handle, NULL);
}

static void close_loop(uv_loop_t *loop) {
  uv_walk(loop, close_walk_cb, NULL);
  uv_run(loop, UV_RUN_DEFAULT);
}

void srv_on_sigint(uv_signal_t *handle, int signum) {
  (void)(signum);
  close_loop(handle->loop);
}

int srv_run(srv_i srv) {
  int ret;

  uv_loop_t loop;

  srv->loop = &loop;

  ret = uv_loop_init(&loop);
  if (ret < 0) {
    goto done;
  }

  ret = uv_tcp_init(&loop, &srv->listen_socket);
  if (ret < 0) {
    goto done;
  }

  ret = uv_ip4_addr(srv->host, srv->port, &srv->addr);
  if (ret < 0) {
    goto done;
  }

  srv->listen_socket.data = srv;

  ret =
      uv_tcp_bind(&srv->listen_socket, (const struct sockaddr *)&srv->addr, 0);
  if (ret < 0) {
    goto done;
  }

  ret = uv_listen((uv_stream_t *)&srv->listen_socket, srv->backlog,
                  srv_connection_cb);
  if (ret < 0) {
    goto done;
  }

  uv_signal_t sigint;
  uv_signal_init(&loop, &sigint);

  uv_signal_start(&sigint, srv_on_sigint, SIGINT);

  uv_run(&loop, UV_RUN_DEFAULT);

done:
  close_loop(&loop);
  assert(0 == uv_loop_close(&loop));
  uv_library_shutdown();

  return ret;
}
