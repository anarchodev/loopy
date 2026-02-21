#include "log.h"
#include <stdio.h>

void log_set_level() {}

static void __log(enum log_level lvl, const char *msg) {
    (void)(lvl);
  printf("%s\n", msg);
  fflush(stdout);
}

void log_info(const char *msg) { __log(LOG_INFO, msg); }
void log_debug(const char *msg) { __log(LOG_DEBUG, msg); }

void log_warn(const char *msg) { __log(LOG_WARN, msg); }

void log_error(const char *msg) { __log(LOG_ERROR, msg); }
