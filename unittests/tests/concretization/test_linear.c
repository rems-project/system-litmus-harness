#include <stdint.h>

#include "lib.h"
#include "testlib.h"

#define SIZE_OF_TEST 100


uint64_t* __var(test_ctx_t* ctx, uint64_t r, const char* varname) {
  uint64_t idx = idx_from_varname(ctx, varname);
  return ctx->heap_vars[idx].values[r];
}
#define VAR(ctx, r, var) __var(ctx, r, var)

void concretize_linear_all(test_ctx_t* ctx, const litmus_test_t* cfg, void* st, int no_runs);
void* concretize_linear_init(test_ctx_t* ctx, const litmus_test_t* cfg, int no_runs);

UNIT_TEST(test_concretization_linear_default_diff_pages)
void test_concretization_linear_default_diff_pages(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    3,(const char*[]){"x","y","z","a"},
    3,(const char*[]){"r", "s", "t"},
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  /* each var must be in its own page */
  for (int r = 0; r < ctx.no_runs; r++) {
    for (int v = 0; v < ctx.cfg->no_heap_vars; v++) {
      for (int v2 = v+1; v2 < ctx.cfg->no_heap_vars; v2++) {
        ASSERT(PAGE(ctx.heap_vars[v].values[r]) != PAGE(ctx.heap_vars[v2].values[r]), "page == page");
      }
    }
  }
}

UNIT_TEST(test_concretization_linear_own_pmd)
void test_concretization_linear_own_pmd(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    3,(const char*[]){"x","y","z","a"},
    3,(const char*[]){"r", "s", "t"},
    INIT_STATE(
      2,
      INIT_REGION_OWN(x, REGION_OWN_PMD),
      INIT_REGION_OWN(y, REGION_OWN_PMD),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  for (int r = 0; r < ctx.no_runs; r++) {
    ASSERT(PMD(ctx.heap_vars[0].values[r]) != PMD(ctx.heap_vars[1].values[r]));
  }
}


UNIT_TEST(test_concretization_linear_same_page)
void test_concretization_linear_same_page(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    2,(const char*[]){"x","y"},
    0,NULL,
    INIT_STATE(
      2,
      INIT_REGION_OWN(x, REGION_OWN_PAGE),
      INIT_REGION_PIN(y, x, REGION_SAME_PAGE_OFFSET),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  /* x and y must be in the same page */
  for (int r = 0; r < ctx.no_runs; r++) {
    ASSERT(PAGE(ctx.heap_vars[0].values[r]) == PAGE(ctx.heap_vars[1].values[r]));
  }
}


UNIT_TEST(test_concretization_separate_roots)
void test_concretization_separate_roots(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    4,(const char*[]){"x","y","a","b"},
    0,NULL,
    INIT_STATE(
      4,
      INIT_REGION_OWN(x, REGION_OWN_PAGE),
      INIT_REGION_PIN(a, x, REGION_SAME_PAGE_OFFSET),
      INIT_REGION_OWN(y, REGION_OWN_PAGE),
      INIT_REGION_PIN(b, y, REGION_SAME_PAGE_OFFSET),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  for (int r = 0; r < ctx.no_runs; r++) {
    ASSERT(PAGE(VAR(&ctx, r, "x")) == PAGE(VAR(&ctx, r, "a")), "x not same page as a");
    ASSERT(PAGE(VAR(&ctx, r, "y")) == PAGE(VAR(&ctx, r, "b")), "y not same page as b");
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "y")), "x and y were same page");
  }
}


UNIT_TEST(test_concretization_aliased)
void test_concretization_aliased(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    2,(const char*[]){"x","y"},
    0,NULL,
    INIT_STATE(
      2,
      INIT_REGION_OWN(x, REGION_OWN_PAGE),
      INIT_ALIAS(y, x),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  for (int r = 0; r < ctx.no_runs; r++) {
    /* aliased things cannot be in the same page */
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "y")), "x in same page as y");
    ASSERT(PAGEOFF(VAR(&ctx, r, "x")) == PAGEOFF(VAR(&ctx, r, "y")), "x and y not same offset in pages");
  }
}


UNIT_TEST(test_concretization_unrelated_aliased)
void test_concretization_unrelated_aliased(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    3,(const char*[]){"x","y","z"},
    0,NULL,
    INIT_STATE(
      3,
      INIT_VAR(x, 0),
      INIT_VAR(y, 0),
      INIT_ALIAS(z, x),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  for (int r = 0; r < ctx.no_runs; r++) {
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "y")), "x in same page as y");
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "z")), "x in same page as z");
    ASSERT(PAGE(VAR(&ctx, r, "y")) != PAGE(VAR(&ctx, r, "z")), "y in same page as z");

    ASSERT(PAGEOFF(VAR(&ctx, r, "x")) == PAGEOFF(VAR(&ctx, r, "z")), "x and y not same offset in pages");
  }
}


UNIT_TEST(test_concretization_unmapped)
void test_concretization_unmapped(void) {
  litmus_test_t test = {
    "test",
    0,NULL,
    3,(const char*[]){"x","y","z"},
    0,NULL,
    INIT_STATE(
      3,
      INIT_UNMAPPED(x),
      INIT_VAR(y, 0),
      INIT_VAR(z, 1)
    ),
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST);
  regions_t* region = alloc(sizeof(regions_t));
  ctx.heap_memory = region;
  void* st = concretize_linear_init(&ctx, ctx.cfg, ctx.no_runs);
  concretize_linear_all(&ctx, ctx.cfg, st, ctx.no_runs);

  for (int r = 0; r < ctx.no_runs; r++) {
    /* no other var on any other run got given the same page as x on this run */
    uint64_t xpage = PAGE(VAR(&ctx, r, "x"));
    for (int varidx = 0; varidx < ctx.cfg->no_heap_vars; varidx++) {
      if (varidx != idx_from_varname(&ctx, "x")) {
        for (int r2 = 0; r2 < ctx.no_runs; r2++) {
          ASSERT(PAGE(ctx.heap_vars[varidx].values[r2]) != xpage, "x and %s had same page", ctx.heap_vars[varidx].name);
        }
      }
    }
  }
}