#include <stdint.h>

#include "lib.h"
#include "testlib.h"

static litmus_test_t big_test = {
  "big test",
  0,NULL,
  3,(const char*[]){"x","y","z"},
  3,(const char*[]){"r", "s", "t"},
};

static int overlaps_page(uint64_t* var, uint64_t* other, uint64_t no_runs) {
  uint64_t var_va = (uint64_t)var;
  uint64_t var_pg = (var_va >> 12) << 12;

  uint64_t other_start_va = (uint64_t)other;
  uint64_t other_end_va = (uint64_t)(&other[no_runs-1]);

  uint64_t other_start_pg = (other_start_va >> 12) << 12;
  uint64_t other_end_pg = (other_end_va >> 12) << 12;

  return ((other_start_pg <= var_pg) && (var_pg <= other_end_pg));
}

UNIT_TEST(test_init_test_ctx_non_overlap_heap)
void test_init_test_ctx_non_overlap_heap(void) {
  test_ctx_t ctx;

  init_test_ctx(&ctx, &big_test, 50000UL);

  uint64_t* xs = ctx.heap_vars[0];
  uint64_t* ys = ctx.heap_vars[1];
  uint64_t* zs = ctx.heap_vars[2];

  for (uint64_t i = 0; i < ctx.no_runs; i += 4096UL) {
    ASSERT(! overlaps_page(&xs[i], ys, ctx.no_runs), "X overlaps with Y");
    ASSERT(! overlaps_page(&xs[i], zs, ctx.no_runs), "X overlaps with Z");
    ASSERT(! overlaps_page(&ys[i], zs, ctx.no_runs), "Y overlaps with Z");
  }

  free_test_ctx(&ctx);
}