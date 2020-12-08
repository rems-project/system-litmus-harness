#include <stdint.h>

#include "lib.h"

volatile uint64_t INITIAL_SEED = 0;
volatile uint64_t SEED = 0;

volatile lock_t __rnd_lock;

void init_seed(void) {
  uint64_t seed = read_clk();
  INITIAL_SEED = seed;
  debug("set initial seed = 0x%lx\n", seed);
}

void reset_seed(void) {
  LOCK(&__rnd_lock);
  SEED = INITIAL_SEED;
  UNLOCK(&__rnd_lock);
}

void rand_seed(uint64_t seed) {
  debug("set seed = 0x%lx\n", seed);
  SEED = seed;
}

uint64_t randn(void) {
  LOCK(&__rnd_lock);
  uint64_t st = 1;

  for (int i = 0; i < 64; i++) {
    if (i % 7 == 0 || i % 13 == 0) {
      int x = (SEED >> i) & 0x1;
      st = st ^ x;
    }
  }
  uint64_t new_seed = (SEED << 1) + st;
  SEED = new_seed;
  UNLOCK(&__rnd_lock);
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

void shuffle(void* p, uint64_t szof, uint64_t n) {
  char* arr = p;

  for (int i = 0; i < n; i++) {
    char datum[szof];
    uint64_t x = (randn() % n) * szof;
    uint64_t y = (randn() % n) * szof;

    valloc_memcpy(datum, arr+x, szof);
    valloc_memcpy(arr+x, arr+y, szof);
    valloc_memcpy(arr+y, datum, szof);
  }
}
