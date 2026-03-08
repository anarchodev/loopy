#include "allocator/allocator.h"
#include  "kv.h"
#include "str/str.h"
#include <assert.h>

int main() {
    kv_i kv = kv_new(allocator_default());
    str_t val;
    str_init(allocator_default(), &val, to_slice(""));
    kv_get(kv, to_slice("foo"), &val);
    kv_set(kv, to_slice("foo"), to_slice("bar"));

    kv_get(kv, to_slice("foo"), &val);

    kv_remove(kv, to_slice("foo"));
    str_deinit(allocator_default(), &val);
    str_init(allocator_default(), &val, to_slice(""));
    kv_get(kv, to_slice("foo"), &val);
    assert(val.slice.len == 0);  /* key no longer exists */

    str_deinit(allocator_default(), &val);
    kv_delete(kv);
    return 0;
}
