#include <stdint.h>

#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_free_test_ctx_empty)
void test_free_test_ctx_empty(void) {
  test_ctx_t ctx;
  uint64_t space = valloc_free_size();

  init_test_ctx(&ctx, "<test>", 0, NULL, 0, 0, 0);
  free_test_ctx(&ctx);

  ASSERT(valloc_free_size() == space, "did not free all space");
  ASSERT(mem.freelist == NULL, "non-null freelist");
}

UNIT_TEST(test_free_test_ctx_reuse)
void test_free_test_ctx_reuse(void) {
  test_ctx_t ctx;
  uint64_t space = valloc_free_size();

  init_test_ctx(&ctx, "<test big>", 2, NULL, 2, 2, 5000);
  char* p = alloc(64);
  free_test_ctx(&ctx);

  init_test_ctx(&ctx, "<another>", 2, NULL, 2, 2, 5000);
  free_test_ctx(&ctx);
  free(p);

  ASSERT(valloc_free_size() == space, "did not free all space");
  ASSERT(mem.freelist == NULL, "non-null freelist");
}

UNIT_TEST(test_free_test_ctx_and_page)
void test_free_test_ctx_and_page(void) {
  test_ctx_t ctx;
  uint64_t space = valloc_free_size();

  init_test_ctx(&ctx, "<test big>", 2, NULL, 2, 2, 500000UL);
  uint64_t* p = vmm_alloc_new_idmap_4k();
  vmm_ensure_level(p, 3, 0x4008000UL);
  free_test_ctx(&ctx);
  vmm_free_pgtable(p);

  debug_valloc_status();

  ASSERT(valloc_free_size() == space, "did not free all space");
  ASSERT(mem.freelist == NULL, "non-null freelist");
}