#ifndef allocator_h_INCLUDED
#define allocator_h_INCLUDED

#include <stddef.h>

typedef struct b_allocator{
    void *(*malloc)(size_t);
    void (*free)(void*);
} allocator_t;

allocator_t allocator_default();

#endif
