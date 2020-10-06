#include <stdint.h>

#include "lib.h"

volatile uint64_t INITIAL_SEED = 0;
volatile uint64_t SEED = 0;

volatile lock_t __rnd_lock;

void reset_seed(void) {
  lock(&__rnd_lock);
  SEED = INITIAL_SEED;
  unlock(&__rnd_lock);
}

void rand_seed(uint64_t seed) {
  SEED = seed;
}

uint64_t randn(void) {
  lock(&__rnd_lock);
  uint64_t st = 1;

  for (int i = 0; i < 64; i++) {
    if (i % 7 == 0 || i % 13 == 0) {
      int x = (SEED >> i) & 0x1;
      st = st ^ x;
    }
  }
  uint64_t new_seed = (SEED << 1) + st;
  SEED = new_seed;
  unlock(&__rnd_lock);
  return new_seed;
}

uint64_t randrange(uint64_t low, uint64_t high) {
  uint64_t r = randn();
  if (low == high)
    return low;

  if (high < low)
    fail("! randrange, low (%ld) was larger than high (%ld)", low, high);

  return low + r % (high - low);
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
