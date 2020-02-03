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

void shuffle(uint64_t* arr, int n) {
  for (int i = 0; i < n; i++) {
    int x = randn() % n;
    int y = randn() % n;
    uint64_t temp = arr[x];
    arr[x] = arr[y];
    arr[y] = temp;
  }
}
