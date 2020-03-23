#include <stdint.h>

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
    x *= 10;
    x += ctoi(*s);
    s++;
  }
  return x;
}