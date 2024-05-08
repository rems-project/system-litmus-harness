#include "lib.h"

static u8 b64decodechar(char c) {
  if ('A' <= c && c <= 'Z')
    return c - 'A';
  else if ('a' <= c && c <= 'z')
    return 26 + (c - 'a');
  else if ('0' <= c && c <= '9')
    return 52 + (c - '0');
  else if (c == '+')
    return 62;
  else if (c == '/')
    return 63;
  else
    unreachable();
}

int eat(const char* data, char bite[3]) {
  /* eat 4 bytes of base64 encoded input
   * encoded as ABCD
   * giving us 3 octets of data:
   * laid out like:
   * AAAA_AABB BBBB_CCCC CCDD_DDDD
   */
  char a = b64decodechar(data[0]);
  char b = b64decodechar(data[1]);
  char c = data[2] == '=' ? 0 : b64decodechar(data[2]);
  char d = data[3] == '=' ? 0 : b64decodechar(data[3]);

  bite[0] = (a << 2) | (b >> 4);
  bite[1] = ((b & 0b1111) << 4) | (c >> 2);
  bite[2] = (c & 0b11) << 6 | d;

  if (data[2] == '=')
    return 1;
  else if (data[3] == '=')
    return 2;
  else
    return 3;
}

u64 b64decode(const char* data, u64 len, char* out) {
  int i = 0;
  int j = 0;
  char bite[3];

  do {
    int ate = eat(data + i, bite);

    for (int t = 0; t < 3; t++) {
      out[j + t] = bite[t];
    }

    if (ate < 3)
      return j + ate;

    i += 4;
    j += 3;
  } while (i < len);

  return j;
}