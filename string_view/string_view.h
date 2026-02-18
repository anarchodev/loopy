#ifndef string_view_h_INCLUDED
#define string_view_h_INCLUDED

#include <stddef.h>

typedef struct sv_s {
  const char *ptr;
  size_t len;
}sv_t;


#endif // string_view_h_INCLUDED
