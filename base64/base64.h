#ifndef base64_h_INCLUDED
#define base64_h_INCLUDED

#include <stddef.h>

/* Encodes src into dst. Returns number of chars written (no null terminator).
   dst must be at least ((len + 2) / 3) * 4 bytes. */
size_t base64_encode(const unsigned char *src, size_t len, char *dst);

/* Decodes src into dst. Returns number of bytes written, or -1 on invalid input.
   dst must be at least (len / 4) * 3 + 3 bytes. */
int base64_decode(const char *src, size_t len, unsigned char *dst);

#endif
