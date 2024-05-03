
#include "types.h"
#include "str.h"
#include "abort.h"

u64 strlen(const char* s) {
  u64 i = 0;

  while (*s != '\0') {
    s++;
    i++;
  }

  return i;
}

int strstartswith(char* s1, char* prefix) {
  if (*prefix == '\0')
    return 1;

  if (*s1 != *prefix)
    return 0;

  return strstartswith(s1 + 1, prefix + 1);
}

int strpartition(char* outL, char* outR, char* s, char sep) {
  while (1) {
    if (*s == '\0')
      return 0;

    if (*s != sep) {
      *outL = *s;
      s++;
      outL++;
    } else {
      s++;
      break;
    }
  }

  while (1) {
    *outR = *s;
    s++;
    outR++;

    if (*(s - 1) == '\0')
      return 1;
  }
}

u64 strcmp(const char* s1, const char* s2) {
  if (*s1 == '\0')
    return *s2 == '\0';

  if (*s1 != *s2)
    return 0;

  return strcmp(s1 + 1, s2 + 1);
}

int ctoi_hex(char c) {
  if (c >= '0' && c <= '9') {
    return ((int)c - '0');
  } else if (c >= 'a' && c <= 'f') {
    return ((int)c - 'a');
  } else if (c >= 'A' && c <= 'F') {
    return ((int)c - 'A');
  }

  return 0;
}

int ctoi(char c) {
  if (c >= '0' && c <= '9') {
    return ((int)c - '0');
  }

  return 0;
}

u64 __atoi_hex(char* s) {
  u64 x = 0;
  /* skip 0x */
  s = s + 2;

  while (*s) {
    char c = *s;
    s++;

    x *= 16;
    x += ctoi_hex(c);
  }

  return x;
}

u64 __atoi_dec(char* s) {
  u64 x = 0;
  while (*s) {
    char c = *s;
    s++;

    /* allow separators in integers:
     * atoi("1_000_000") == 1e6
     */
    if (c == '_')
      continue;

    /* allow ending in K/M for automatic *1000 or *1_000_000
     */
    if (c == 'k' || c == 'K') {
      if (*s != '\0') {
        fail("invalid input, [kK] only valid at end.\n");
        return 0;
      }

      return x * 1000L;
    } else if (c == 'm' || c == 'M') {
      if (*s != '\0') {
        fail("invalid input, [mM] only valid at end.\n");
        return 0;
      }

      return x * 1000000L;
    }

    x *= 10;
    x += ctoi(c);
  }
  return x;
}

u64 atoi(char* s) {
  if (*s == '0' && *(s + 1) == 'x')
    return __atoi_hex(s);

  return __atoi_dec(s);
}