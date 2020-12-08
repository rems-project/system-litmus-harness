#include <stdint.h>

#include "lib.h"
#include "testlib.h"


UNIT_TEST(test_valloc_alloc_aligned)
void test_valloc_alloc_aligned(void) {
  char* p = alloc(1024);
  FREE(p);
  ASSERT(IS_ALIGNED((uint64_t)p, 10));
}