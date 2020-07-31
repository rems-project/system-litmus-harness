#include <stdint.h>

#include "abort.h"

uint64_t strlen(char* s) {
  uint64_t i = 0;

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

  return strstartswith(s1+1, prefix+1);
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

    if (*(s-1) == '\0')
      return 1;
  }
}

int strcmp(char* s1, char* s2) {
  if (*s1 == '\0')
    return *s2 == '\0';

  if (*s1 != *s2)
    return 0;

  return strcmp(s1+1, s2+1);
}

int ctoi(char c) {
  if (c >= '0' && c <= '9') {
    return ((int)c - 48);
  }

  return 0;
}

uint64_t atoi(char* s) {
  uint64_t x = 0;
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
      if (*(s+1) != '\0') {
        fail("invalid input, [kK] only valid at end.\n");
        return 0;
      }

      return x * 1000L;
    } else if (c == 'm' || c == 'M') {
      if (*(s+1) != '\0') {
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