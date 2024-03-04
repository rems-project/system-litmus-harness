
#include "lib.h"
#include "testlib.h"

static litmus_test_t small_test = {
  "small test",
  0,NULL,
  0,NULL,
  0,NULL,
};

static litmus_test_t big_test = {
  "big test",
  0,NULL,
  3,(const char*[]){"x","y","z"},
  3,(const char*[]){"r", "s", "t"},
};

UNIT_TEST(test_free_test_ctx_empty)
void test_free_test_ctx_empty(void) {
  test_ctx_t ctx;
  u64 space = valloc_free_size();

  init_test_ctx(&ctx, &small_test, 0, 1);
  free_test_ctx(&ctx);

  ASSERT(valloc_free_size() == space, "did not free all space");
  ASSERT(mem.freelist == NULL, "non-null freelist");
}

UNIT_TEST(test_free_test_ctx_reuse)
void test_free_test_ctx_reuse(void) {
  test_ctx_t ctx;
  u64 space = valloc_free_size();

  init_test_ctx(&ctx, &big_test, 5000, 1);
  char* p = alloc(64);
  free_test_ctx(&ctx);

  init_test_ctx(&ctx, &big_test, 5000, 1);
  free_test_ctx(&ctx);
  FREE(p);

  ASSERT(valloc_free_size() == space, "did not free all space");
  ASSERT(mem.freelist == NULL, "non-null freelist");
}