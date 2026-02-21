#include <stdlib.h>
#include "allocator.h"

allocator_t allocator_default() {
    allocator_t def;
    def.free = &free;
    def.malloc = &malloc;
    def.realloc =  &realloc;

    return def;
}
