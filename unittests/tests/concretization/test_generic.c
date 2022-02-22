
#include "lib.h"
#include "testlib.h"

#define SIZE_OF_TEST 100


u64* __var(test_ctx_t* ctx, run_idx_t r, const char* varname) {
  var_idx_t idx = idx_from_varname(ctx, varname);
  return ctx->heap_vars[idx].values[r];
}

#define VAR(ctx, r, var) __var(ctx, r, var)

UNIT_TEST(test_concretization_linear_default_diff_pages)
UNIT_TEST(test_concretization_random_default_diff_pages)
void __test_concretization_default_diff_pages(concretize_type_t conc_type) {
  litmus_test_t test = {
    "test",
    0,NULL,
    3,(const char*[]){"x","y","z","a"},
    3,(const char*[]){"r", "s", "t"},
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  /* each var must be in its own page */
  for (int r = 0; r < ctx.no_runs; r++) {
    for (int v = 0; v < ctx.cfg->no_heap_vars; v++) {
      for (int v2 = v+1; v2 < ctx.cfg->no_heap_vars; v2++) {
        ASSERT(PAGE(ctx.heap_vars[v].values[r]) != PAGE(ctx.heap_vars[v2].values[r]), "page == page");
      }
    }
  }
}

void test_concretization_linear_default_diff_pages(void) {
  __test_concretization_default_diff_pages(CONCRETE_LINEAR);
}

void test_concretization_random_default_diff_pages(void) {
  __test_concretization_default_diff_pages(CONCRETE_RANDOM);
}

UNIT_TEST(test_concretization_linear_own_pmd)
UNIT_TEST(test_concretization_random_own_pmd)
void __test_concretization_own_pmd(concretize_type_t conc_type) {
  litmus_test_t test = {
    "test",
    0,NULL,
    2,(const char*[]){"x","y"},
    0,(const char*[]){},
    INIT_STATE(
      2,
      INIT_REGION_OWN(x, REGION_OWN_PMD),
      INIT_REGION_OWN(y, REGION_OWN_PMD),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    u64* x = VAR(&ctx, r, "x");
    u64* y = VAR(&ctx, r, "y");

    u64 xpmd = PMD(x);
    u64 ypmd = PMD(y);

    ASSERT(xpmd != ypmd, "x (%p) and y (%p) were placed in the same pmd", x, y);
  }
}

void test_concretization_linear_own_pmd(void) {
  __test_concretization_own_pmd(CONCRETE_LINEAR);
}

void test_concretization_random_own_pmd(void) {
  __test_concretization_own_pmd(CONCRETE_RANDOM);
}


UNIT_TEST(test_concretization_linear_same_page)
UNIT_TEST(test_concretization_random_same_page)
void __test_concretization_same_page(concretize_type_t conc_type) {
  litmus_test_t test = {
    "test",
    0,NULL,
    2,(const char*[]){"x","y"},
    0,NULL,
    INIT_STATE(
      2,
      INIT_REGION_OWN(x, REGION_OWN_PAGE),
      INIT_REGION_PIN(y, x, REGION_SAME_PAGE),
    )
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  /* x and y must be in the same page */
  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    ASSERT(PAGE(ctx.heap_vars[0].values[r]) == PAGE(ctx.heap_vars[1].values[r]));
  }
}

void test_concretization_linear_same_page(void) {
  __test_concretization_same_page(CONCRETE_LINEAR);
}

void test_concretization_random_same_page(void) {
  __test_concretization_same_page(CONCRETE_RANDOM);
}


UNIT_TEST(test_concretization_linear_separate_roots)
UNIT_TEST(test_concretization_random_separate_roots)
void __test_concretization_separate_roots(concretize_type_t conc_type) {
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

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    ASSERT(PAGE(VAR(&ctx, r, "x")) == PAGE(VAR(&ctx, r, "a")), "x not same page as a");
    ASSERT(PAGE(VAR(&ctx, r, "y")) == PAGE(VAR(&ctx, r, "b")), "y not same page as b");
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "y")), "x and y were same page");
  }
}

void test_concretization_linear_separate_roots(void) {
  __test_concretization_separate_roots(CONCRETE_LINEAR);
}

void test_concretization_random_separate_roots(void) {
  __test_concretization_separate_roots(CONCRETE_RANDOM);
}

UNIT_TEST(test_concretization_linear_aliased)
UNIT_TEST(test_concretization_random_aliased)
void __test_concretization_aliased(concretize_type_t conc_type) {
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

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    /* aliased things cannot be in the same page */
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "y")), "x in same page as y");
    ASSERT(PAGEOFF(VAR(&ctx, r, "x")) == PAGEOFF(VAR(&ctx, r, "y")), "x and y not same offset in pages");
  }
}

void test_concretization_linear_aliased(void) {
  __test_concretization_aliased(CONCRETE_LINEAR);
}

void test_concretization_random_aliased(void) {
  __test_concretization_aliased(CONCRETE_RANDOM);
}

UNIT_TEST(test_concretization_linear_unrelated_aliased)
UNIT_TEST(test_concretization_random_unrelated_aliased)
void __test_concretization_unrelated_aliased(concretize_type_t conc_type) {
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

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "y")), "x in same page as y");
    ASSERT(PAGE(VAR(&ctx, r, "x")) != PAGE(VAR(&ctx, r, "z")), "x in same page as z");
    ASSERT(PAGE(VAR(&ctx, r, "y")) != PAGE(VAR(&ctx, r, "z")), "y in same page as z");

    ASSERT(PAGEOFF(VAR(&ctx, r, "x")) == PAGEOFF(VAR(&ctx, r, "z")), "x and y not same offset in pages");
  }
}

void test_concretization_linear_unrelated_aliased(void) {
  __test_concretization_unrelated_aliased(CONCRETE_LINEAR);
}

void test_concretization_random_unrelated_aliased(void) {
  __test_concretization_unrelated_aliased(CONCRETE_RANDOM);
}


UNIT_TEST(test_concretization_linear_unmapped)
UNIT_TEST(test_concretization_random_unmapped)
void __test_concretization_unmapped(concretize_type_t conc_type) {
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

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    /* no other var on any other run got given the same page as x on this run */
    u64* x = VAR(&ctx, r, "x");
    u64 xpage = PAGE(x);
    for (var_idx_t varidx = 0; varidx < ctx.cfg->no_heap_vars; varidx++) {
      if (varidx != idx_from_varname(&ctx, "x")) {
        for (run_idx_t r2 = 0; r2 < ctx.no_runs; r2++) {
          u64* v2 = ctx.heap_vars[varidx].values[r2];
          u64 v2page = PAGE(v2);
          ASSERT(v2page != xpage, "x (%p) and %s (%p) had same page", x, ctx.heap_vars[varidx].name, v2);
        }
      }
    }
  }
}

void test_concretization_linear_unmapped(void) {
  __test_concretization_unmapped(CONCRETE_LINEAR);
}

void test_concretization_random_unmapped(void) {
  __test_concretization_unmapped(CONCRETE_RANDOM);
}


UNIT_TEST(test_concretization_linear_twopage)
UNIT_TEST(test_concretization_random_twopage)
void __test_concretization_twopage(concretize_type_t conc_type) {
  litmus_test_t test = {
    "test",
    0,NULL,
    4,(const char*[]){"a1", "a2", "b1", "b2"},
    0,NULL,
    INIT_STATE(
      4,
      INIT_REGION_OWN(a1, REGION_OWN_PAGE),
      INIT_REGION_OWN(b1, REGION_OWN_PAGE),
      INIT_REGION_PIN(a2, a1, REGION_SAME_PAGE),
      INIT_REGION_PIN(b2, b1, REGION_SAME_PAGE),
    ),
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    /* no other var on any other run got given the same page as x on this run */
    u64 a1page = PAGE(VAR(&ctx, r, "a1"));
    u64 a2page = PAGE(VAR(&ctx, r, "a2"));
    u64 b1page = PAGE(VAR(&ctx, r, "b1"));
    u64 b2page = PAGE(VAR(&ctx, r, "b2"));

    ASSERT(a1page==a2page, "a1 and a2 weren't same page");
    ASSERT(b1page==b2page, "b1 and b2 weren't same page");
    ASSERT(a1page!=b1page, "b1 and a1 were same page");
  }
}

void test_concretization_linear_twopage(void) {
  __test_concretization_twopage(CONCRETE_LINEAR);
}

void test_concretization_random_twopage(void) {
  __test_concretization_twopage(CONCRETE_RANDOM);
}



UNIT_TEST(test_concretization_linear_relpmdoverlap)
UNIT_TEST(test_concretization_random_relpmdoverlap)
void __test_concretization_relpmdoverlap(concretize_type_t conc_type) {
  litmus_test_t test = {
    "test",
    0,NULL,
    2,(const char*[]){"x", "y"},
    0,NULL,
    INIT_STATE(
      2,
      INIT_REGION_OWN(x, REGION_OWN_PMD),
      INIT_REGION_OFFSET(y, x, REGION_SAME_PMD_OFFSET),
    ),
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    /* no other var on any other run got given the same page as x on this run */
    u64 xpmd = PMD(VAR(&ctx, r, "x"));
    u64 ypmd = PMD(VAR(&ctx, r, "y"));

    u64 xpmdoff = PMDOFF(VAR(&ctx, r, "x"));
    u64 ypmdoff = PMDOFF(VAR(&ctx, r, "y"));

    ASSERT(xpmd != ypmd, "x and y were same pmd");
    ASSERT(xpmdoff == ypmdoff, "x and y had different offsets");
  }
}

void test_concretization_linear_relpmdoverlap(void) {
  __test_concretization_relpmdoverlap(CONCRETE_LINEAR);
}

void test_concretization_random_relpmdoverlap(void) {
  __test_concretization_relpmdoverlap(CONCRETE_RANDOM);
}


UNIT_TEST(test_concretization_linear_multi_pmd_pin)
UNIT_TEST(test_concretization_random_multi_pmd_pin)
void __test_concretization_multi_pmd_pin(concretize_type_t conc_type) {
  litmus_test_t test = {
    "test",
    0,NULL,
    3,(const char*[]){"x", "y", "z"},
    0,NULL,
    INIT_STATE(
      8,
      INIT_VAR(x, 0),
      INIT_VAR(y, 1),
      INIT_VAR(z, 2),
      INIT_REGION_OWN(x, REGION_OWN_PMD),
      INIT_REGION_PIN(y, x, REGION_SAME_PMD),
      INIT_REGION_OFFSET(y, x, REGION_SAME_PAGE_OFFSET),
      INIT_REGION_OWN(z, REGION_OWN_PMD),
      INIT_REGION_OFFSET(z, x, REGION_SAME_PMD_OFFSET),
    ),
  };

  test_ctx_t ctx;

  init_test_ctx(&ctx, &test, SIZE_OF_TEST, 1);
  initialize_regions(&ctx.heap_memory);
  void* st = concretize_init(conc_type, &ctx, ctx.cfg, ctx.no_runs);
  concretize(conc_type, &ctx, ctx.cfg, st, ctx.no_runs);

  for (run_idx_t r = 0; r < ctx.no_runs; r++) {
    /* no other var on any other run got given the same page as x on this run */
    u64* x = VAR(&ctx, r, "x");
    u64* y = VAR(&ctx, r, "y");
    u64* z = VAR(&ctx, r, "z");

    u64 xpmd = PMD(x);
    u64 ypmd = PMD(y);
    u64 zpmd = PMD(z);

    u64 xpmdoff = PMDOFF(x);
    u64 zpmdoff = PMDOFF(z);

    u64 xpageoffs = PAGEOFF(x);
    u64 ypageoffs = PAGEOFF(y);
    u64 zpageoffs = PAGEOFF(z);

    ASSERT(xpmd != zpmd, "x (%p) and z (%p) were placed in same pmd", x, z);
    ASSERT(xpmd == ypmd, "x and y were not pinned to the same pmd");
    ASSERT(xpmdoff == zpmdoff, "x and z did not have same pmd offset");
    ASSERT(xpageoffs == ypageoffs, "x and y did not have same page offset");
    ASSERT(xpageoffs == zpageoffs, "x and z did not have same page offset");
  }
}

void test_concretization_linear_multi_pmd_pin(void) {
  __test_concretization_multi_pmd_pin(CONCRETE_LINEAR);
}

void test_concretization_random_multi_pmd_pin(void) {
  __test_concretization_multi_pmd_pin(CONCRETE_RANDOM);
}