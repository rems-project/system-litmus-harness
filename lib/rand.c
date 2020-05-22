#include <stdint.h>

#include "lib.h"

void rand_seed(uint64_t seed) {
  SEED = seed;
}

uint64_t randn(void) {
  uint64_t st = 1;

  for (int i = 0; i < 64; i++) {
    if (i % 7 == 0 || i % 13 == 0) {
      int x = (SEED >> i) & 0x1;
      st = st ^ x;
    }
  }
  SEED = (SEED << 1) + st;
  return SEED;
}

void shuffle(void* p, int szof, int n) {
  char* arr = p;

  for (int i = 0; i < n/szof; i++) {
    char datum[szof];
    int x = (randn() % n) * szof;
    int y = (randn() % n) * szof;

    valloc_memcpy(datum, arr+x, szof);
    valloc_memcpy(arr+x, arr+y, szof);
    valloc_memcpy(arr+y, datum, szof);
  }
}
