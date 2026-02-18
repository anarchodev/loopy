#include "serve.h"
#include "allocator/allocator.h"
#include "serve_internal.h"
#include "uv.h"
#include <assert.h>
#include <string.h>

srv_i srv_new(allocator_t allocator, srv_options_t options) {
  srv_t *srv = allocator.malloc(sizeof(*srv));

  srv->allocator = allocator;
  srv->options = options;

  return srv;
}

void srv_delete(srv_i srv) { srv->allocator.free(srv); }

void free_buf(allocator_t alloc, const uv_buf_t *buf) {
  if (buf->len > 0)
    alloc.free(buf->base);
}

void close_cb(uv_handle_t *) {}
void alloc_buf(uv_handle_t *stream, size_t suggested_size, uv_buf_t *buf) {
  srv_request_t *request = stream->data;
  allocator_t alloc = request->allocator;
  buf->base = alloc.malloc(suggested_size);
  buf->len = suggested_size;
}

void write_cb(uv_write_t *req, int) {
  srv_response_t *res = req->data;
  res->allocator.free(res);
}

srv_request_t *srv_request_new(allocator_t allocator, srv_request_cb cb) {
  srv_request_t *request = allocator.malloc(sizeof(*request));
  request->headers_count =
      sizeof(request->headers) / sizeof(request->headers[0]);
  request->allocator = allocator;
  request->request_cb = cb;
  return request;
}

void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  srv_request_t *request = stream->data;
  allocator_t alloc = request->allocator;
  if (nread < 0)
    goto done;
  if (nread == UV_EOF)
    goto done;

  if (nread + request->current_buffer.len >
      (long)sizeof(request->current_buffer.ptr)) {
    // to big
    goto done;
  }

  memcpy(request->buffer + request->current_buffer.len, buf->base, nread);

  request->buffer_previous.len = request->current_buffer.len;
  request->current_buffer.len = request->current_buffer.len + nread;

  int ret = phr_parse_request(
      request->buffer, request->current_buffer.len, &request->method.ptr,
      &request->method.len, &request->path.ptr, &request->path.len,
      &request->minor_version, request->headers, &request->headers_count,
      request->buffer_previous.len);

  if (ret > 0) {
    // done
    printf("buf_len: %.*s\n", (int)request->current_buffer.len,
           request->buffer);
    fflush(stdout);

    srv_response_t *response = alloc.malloc(sizeof(*response));
    response->allocator = alloc;

    if (request->request_cb) {
      request->request_cb(request, response);

      uv_write_t *write = alloc.malloc(sizeof(*write));
      write->data = response;

      uv_buf_t buf;
      buf = uv_buf_init((char *)response->buffer.ptr, response->buffer.len);

      uv_write(write, stream, &buf, 1, write_cb);

      goto done;
    }

    uv_close((uv_handle_t *)stream, NULL);

    goto done;

  } else if (ret == -1) {
    goto done;
  }

  free_buf(alloc, buf);

  return;

done:
  free_buf(alloc, buf);
  uv_close((uv_handle_t *)stream, close_cb);
  uv_read_stop(stream);
}

void connection_cb(uv_stream_t *stream, int status) {
  printf("sdasdf");
  fflush(stdout);
  if (status < 0) {
    printf("failed to conect to client");
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

  if ((status = uv_read_start((uv_stream_t *)client, alloc_buf, read_cb)) < 0) {
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

  ret = uv_ip4_addr(srv->options.host, srv->options.port, &srv->addr);
  if (ret < 0) {
    goto done;
  }

  srv->listen_socket.data = srv;

  ret =
      uv_tcp_bind(&srv->listen_socket, (const struct sockaddr *)&srv->addr, 0);
  if (ret < 0) {
    goto done;
  }

  ret = uv_listen((uv_stream_t *)&srv->listen_socket, srv->options.backlog,
                  connection_cb);
  if (ret < 0) {
    goto done;
  }

  uv_run(&loop, UV_RUN_DEFAULT);

done:
  close_loop(&loop);
  assert(0 == uv_loop_close(&loop));
  uv_library_shutdown();

  return ret;
}
