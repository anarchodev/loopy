#ifndef serve_h_INCLUDED
#define serve_h_INCLUDED

#include "allocator/allocator.h"
#include "str/str.h"

typedef struct srv_response_s *srv_response_i;
typedef struct srv_request_s *srv_request_i;

typedef void(*srv_request_cb)(srv_request_i, srv_response_i);

typedef struct srv_options_s {
  const char *host;
  int port;
  int backlog;
  srv_request_cb request_cb;
} srv_options_t;

typedef struct srv_s *srv_i;

srv_i srv_new(allocator_t, srv_options_t);
void srv_delete(srv_i);
int srv_run(srv_i);

srv_options_t srv_default();

void srv_response_status_set(srv_response_i,int status);
int srv_response_body_append(srv_response_i, str_slice_t body);
int srv_response_headers_append(srv_response_i, str_slice_t header);

#endif
