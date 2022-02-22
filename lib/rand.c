
#include "lib.h"

volatile u64 INITIAL_SEED = 0;
volatile u64 SEED = 0;

volatile lock_t __rnd_lock;

void init_seed(void) {
  u64 seed = read_clk();
  INITIAL_SEED = seed;
  debug("set initial seed = 0x%lx\n", seed);
}

void reset_seed(void) {
  LOCK(&__rnd_lock);
  SEED = INITIAL_SEED;
  UNLOCK(&__rnd_lock);
}

void rand_seed(u64 seed) {
  debug("set seed = 0x%lx\n", seed);
  SEED = seed;
}

u64 randn(void) {
  LOCK(&__rnd_lock);
  u64 st = 1;

  for (int i = 0; i < 64; i++) {
    if (i % 7 == 0 || i % 13 == 0) {
      int x = (SEED >> i) & 0x1;
      st = st ^ x;
    }
  }
  u64 new_seed = (SEED << 1) + st;
  SEED = new_seed;
  UNLOCK(&__rnd_lock);
  return new_seed;
}

u64 randrange(u64 low, u64 high) {
  u64 r = randn();
  if (low == high)
    return low;

  if (high < low)
    fail("! randrange, low (%ld) was larger than high (%ld)", low, high);

  return low + r % (high - low);
}

void shuffle(void* p, u64 szof, u64 n) {
  char* arr = p;

  for (int i = 0; i < n; i++) {
    char datum[szof];
    u64 x = (randn() % n) * szof;
    u64 y = (randn() % n) * szof;

    valloc_memcpy(datum, arr+x, szof);
    valloc_memcpy(arr+x, arr+y, szof);
    valloc_memcpy(arr+y, datum, szof);
  }
}
