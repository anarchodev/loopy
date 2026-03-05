#include <allocator/allocator.h>
#include <assert.h>
#include <log/log.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <uv.h>

#include "serve.h"
#include "serve_internal.h"
#include "str/str.h"

srv_i srv_new(allocator_t allocator, srv_options_t options) {
  srv_t *srv = allocator.malloc(sizeof(*srv));
  if (srv == NULL)
    return NULL;

  srv->allocator = allocator;
  srv->request_cb = options.request_cb;
  srv->port = options.port;
  srv->host = options.host;
  srv->backlog = options.backlog;

  return srv;
}

void close_connection_cb(uv_handle_t *stream) {
  srv_request_t *req = stream->data;
  req->allocator.free(stream);
  req->allocator.free(req);
}

void srv_delete(srv_i srv) { srv->allocator.free(srv); }

void free_buf(allocator_t alloc, const uv_buf_t *buf) {
  if (buf->len > 0)
    alloc.free(buf->base);
}

void alloc_buf(uv_handle_t *stream, size_t suggested_size, uv_buf_t *buf) {
  srv_request_t *request = stream->data;
  allocator_t alloc = request->allocator;
  buf->base = alloc.malloc(suggested_size);
  buf->len = buf->base != NULL ? suggested_size : 0;
}

void srv_write_cb(uv_write_t *req, int) {
  srv_response_t *res = req->data;
  allocator_t alloc = res->allocator;
  alloc.free(res);
  alloc.free(req);
}

static const char *reason_phrase(int status) {
  switch (status) {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 304: return "Not Modified";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 409: return "Conflict";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 503: return "Service Unavailable";
    default:  return "Unknown";
  }
}

static size_t find_content_length(struct phr_header *headers, size_t count) {
  size_t i;
  for (i = 0; i < count; i++) {
    if (strncasecmp(headers[i].name, "content-length", headers[i].name_len) == 0)
      /* safe: picohttpparser points value into the raw buffer where
       * the character immediately after value_len is always '\r' */
      return (size_t)strtoul(headers[i].value, NULL, 10);
  }
  return 0;
}

int srv_request_parse(srv_request_t *self) {
  size_t num_headers = sizeof(self->headers) / sizeof(self->headers[0]);
  int ret = phr_parse_request(self->raw_buffer, str_fixed_len(&self->buffer),
                              (const char **)&self->method.ptr, &self->method.len,
                              (const char **)&self->path.ptr, &self->path.len,
                              &self->minor_version, self->headers,
                              &num_headers, self->buffer_previous_len);
  if (ret > 0)
    self->headers_count = num_headers;
  return ret;
}

void srv_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  srv_request_t *request = stream->data;
  allocator_t alloc = request->allocator;
  if (nread < 0)
    goto done;

  request->buffer_previous_len = str_fixed_len(&request->buffer);
  if (str_fixed_append(&request->buffer, str_slice(buf->base, nread)) < 0) {
      goto done;
  }

  if (!request->headers_parsed) {
    int ret = srv_request_parse(request);
    if (ret <= 0) {
      free_buf(alloc, buf);
      return;
    }
    request->headers_parsed = 1;
    request->body.ptr = request->raw_buffer + ret;
    request->expected_body_len =
        find_content_length(request->headers, request->headers_count);
  }

  request->body.len =
      str_fixed_len(&request->buffer) - (size_t)(request->body.ptr - request->raw_buffer);

  if (request->body.len >= request->expected_body_len) {
    request->body.len = request->expected_body_len;
    srv_response_t *response = srv_response_new(alloc);

    if (request->request_cb) {
      request->request_cb(request, response);

      uv_write_t *write = alloc.malloc(sizeof(*write));
      if (write == NULL) {
        srv_response_delete(response);
        goto done;
      }
      write->data = response;

      uv_buf_t bufs[3];

      const char *reason = reason_phrase(response->status);
      str_fixed_append(&response->status_line, to_slice("HTTP/1.1 "));
      str_fixed_append_long(&response->status_line, response->status);
      str_fixed_append(&response->status_line, to_slice(" "));
      str_fixed_append(&response->status_line,
                       str_cstring_to_slice(reason, strlen(reason)));
      str_fixed_append(&response->status_line, to_slice("\r\n"));

      str_fixed_append(&response->headers, to_slice("Content-Length: "));
      str_fixed_append_long(&response->headers, response->body.s.len);
      str_fixed_append(&response->headers, to_slice("\r\n\r\n"));

      bufs[0] =
          uv_buf_init(response->status_line.s.ptr, response->status_line.s.len);
      bufs[1] = uv_buf_init(response->headers.s.ptr, response->headers.s.len);
      bufs[2] = uv_buf_init(response->body.s.ptr, response->body.s.len);

      uv_write(write, stream, bufs, 3, srv_write_cb);
    } else {
      srv_response_delete(response);
    }
    srv_request_reset(request);
  }

  free_buf(alloc, buf);
  return;

done:
  free_buf(alloc, buf);
  uv_read_stop(stream);
  uv_close((uv_handle_t *)stream, close_connection_cb);
}

void srv_connection_cb(uv_stream_t *stream, int status) {
  if (status < 0) {
    log_error("failed to connect");
    return;
  }
  srv_t *srv = stream->data;
  allocator_t alloc = srv->allocator;
  srv_request_t *request = srv_request_new(alloc, srv->request_cb);
  if (request == NULL)
    return;
  uv_tcp_t *client = alloc.malloc(sizeof(*client));
  if (client == NULL) {
    srv_request_delete(request);
    return;
  }
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
  srv_request_delete(request);
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
  uv_stop(handle->loop);
}

int srv_run(srv_i srv) {
  int ret;

  uv_loop_t loop;

  srv->loop = &loop;

  ret = uv_loop_init(&loop);
  if (ret < 0) {
    uv_library_shutdown();
    return ret;
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
