#include <stdint.h>

#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_free_test_ctx)
void test_free_test_ctx(void) {
  test_ctx_t ctx;
  uint64_t space = valloc_free_size();

  init_test_ctx(&ctx, "<test>", 0, NULL, 0, 0, 0);
  free_test_ctx(&ctx);

  ASSERT(valloc_free_size() == space, "did not free all space");
  ASSERT(mem.freelist == NULL, "non-null freelist");
}