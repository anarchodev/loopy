#include "allocator/allocator.h"
#include "js/js.h"
#include "kv/kv.h"
#include "str/str.h"
#include <assert.h>

static void test_js_new_delete(void) {
  js_options_t options;
  allocator_t a;
  a = allocator_default();
  kv_i kv = kv_new(a);
  options.kv = kv;
  str_init(a, &options.kv_prefix, to_slice("__SYSTEM__"));

  assert(js_run(a, options));
  kv_delete(kv);
}

int main(void) {
  test_js_new_delete();
  return 0;
}
