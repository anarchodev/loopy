#include "allocator/allocator.h"
#include "str/str.h"
#include <assert.h>
#include <stdlib.h>

int main() {
  str_dynamic_t str;
  char *blah = malloc(MAX_DYNAMIC_BUFFER);

  str_dynamic_new(allocator_default(), &str, str_cstring_to_slice("a", 1));

  str_dynamic_append(&str, str_cstring_to_slice("b", 1));
  str_dynamic_append(&str, str_cstring_to_slice("c", 1));

  assert(str.slice.len == 3);

  assert(-1 == str_dynamic_append(
                   &str, str_cstring_to_slice(blah, MAX_DYNAMIC_BUFFER)));


  free(blah);

  assert(str.slice.len == 3);

  return 0;
}
