#ifndef log_h_INCLUDED
#define log_h_INCLUDED

enum log_level {
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG
};

void log_set_level();

void log_info(const char *msg);
void log_debug(const char *msg);
void log_warn(const char *msg);
void log_error(const char *msg);

#endif
