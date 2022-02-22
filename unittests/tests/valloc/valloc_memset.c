
#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_valloc_memset_dc_zvz)
void test_valloc_memset_dc_zvz(void) {
 for (int k = 0; k < 1000; k++) {
    u64 count = randrange(10, 1024);
    u8* vars = ALLOC_MANY(u8, count);
    u64 lower = randrange(0, count);
    u64 size = randrange(0, count - lower);
    u64 upper = lower + size;

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
