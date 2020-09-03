#include <stdint.h>

#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_valloc_memset_dc_zvz)
void test_valloc_memset_dc_zvz(void) {
 for (int k = 0; k < 1000; k++) {
    uint64_t count = randrange(10, 1024);
    uint8_t* vars = ALLOC_MANY(uint8_t, count);
    uint64_t lower = randrange(0, count);
    uint64_t size = randrange(0, count - lower);
    uint64_t upper = lower + size;

    ASSERT(lower < count, "lower was >= count");
    ASSERT(size < count - lower + 1, "size >= count - lower");
    ASSERT(upper < count, "upper went over");

    valloc_memset(vars, 1, count);
    valloc_memset(vars+lower, 0, size);

    for (int i = 0; i < count; i++ ) {
      if (lower <= i && i < upper) {
        ASSERT(vars[i] == 0);
      } else {
        ASSERT(vars[i] != 0);
      }
    }

    free(vars);
  }
}
