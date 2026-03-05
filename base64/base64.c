#include "base64.h"

static const signed char b64dec_table[256] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
  52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
  15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
  -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

static const char b64enc_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t base64_encode(const unsigned char *src, size_t len, char *dst) {
  size_t i;
  size_t out = 0;
  unsigned long v;
  for (i = 0; i + 2 < len; i += 3) {
    v = ((unsigned long)src[i] << 16) | ((unsigned long)src[i+1] << 8) | src[i+2];
    dst[out++] = b64enc_table[(v >> 18) & 63];
    dst[out++] = b64enc_table[(v >> 12) & 63];
    dst[out++] = b64enc_table[(v >>  6) & 63];
    dst[out++] = b64enc_table[ v        & 63];
  }
  if (i < len) {
    v = (unsigned long)src[i] << 16;
    if (i + 1 < len) v |= (unsigned long)src[i+1] << 8;
    dst[out++] = b64enc_table[(v >> 18) & 63];
    dst[out++] = b64enc_table[(v >> 12) & 63];
    dst[out++] = (i + 1 < len) ? b64enc_table[(v >> 6) & 63] : '=';
    dst[out++] = '=';
  }
  return out;
}

int base64_decode(const char *src, size_t len, unsigned char *dst) {
  size_t i;
  int out = 0;
  unsigned long acc = 0;
  int bits = 0;
  signed char v;
  for (i = 0; i < len; i++) {
    if (src[i] == '=') break;
    v = b64dec_table[(unsigned char)src[i]];
    if (v < 0) return -1;
    acc = (acc << 6) | (unsigned long)v;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      dst[out++] = (unsigned char)(acc >> bits);
    }
  }
  return out;
}
