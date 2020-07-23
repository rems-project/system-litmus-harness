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