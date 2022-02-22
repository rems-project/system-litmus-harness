
#include "lib.h"
#include "testlib.h"


UNIT_TEST(test_valloc_alloc_aligned)
void test_valloc_alloc_aligned(void) {
  char* p = alloc(1024);
  FREE(p);
  ASSERT(IS_ALIGNED((u64)p, 10));
}