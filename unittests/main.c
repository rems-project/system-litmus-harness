#include "lib.h"
#include "testlib.h"

#include "tests.externs"

/* this needs to be here as it may be > stack size
 */
test_file_t tests[] = {
  #include "tests.cstruct"
};

unit_test_t* current_test;

argdef_t *THIS_ARGS = &UNITTEST_ARGS;

void main(void) {
  unsigned int total_count = 0;
  unsigned int total_failure = 0;
  unsigned int total_skipped = 0;

  for (int fidx = 0; fidx < NO_TEST_FILES; fidx++) {
    test_file_t* f = &tests[fidx];
    printf("--- %s ---\n", f->name);

    for (int tidx = 0; tidx < f->no_tests; tidx++) {
      /* horrible hack to "reset" memory allocations back to where it was before
       */
      valloc_mempool pool = mem;
      unit_test_t* fn = f->fns[tidx];
      current_test = fn;
      trace("# %s\n", fn->name);

      /* if condition not set */
      if (fn->cond && !*fn->cond) {
        total_skipped++;
        trace("skipped\n");
        continue;
      }

      fn->fn();
      if (ENABLE_PGTABLE) {
        vmm_ensure_in_harness_pgtable_ctx(); /* ensure mmu is on, incase the test fn switched it off */
      }
      mem = pool;
      total_count++;
      if (fn->result) {
        printf(".");
        fn->result = 1;
      } else {
        total_failure++;
        printf("F");
      }
      if (TRACE)
        printf("\n");
    }

    printf("\n");
  }

  printf("total: %d failures of %d runs (%d skipped)\n", total_failure, total_count, total_skipped);

  if (total_failure > 0) {
    printf("Failures: \n");
    for (int fidx = 0; fidx < NO_TEST_FILES; fidx++) {
      test_file_t* f = &tests[fidx];
      for (int tidx = 0; tidx < f->no_tests; tidx++) {
        unit_test_t* fn = f->fns[tidx];
        if (fn->cond && !*fn->cond) {
          /* skipped */
          continue;
        }
        if (fn->result == 0) {
          if (fn->msg) {
            printf("* [%s] %s : %s\n", f->name, fn->name, fn->msg);
          } else {
            printf("* [%s] %s\n", f->name, fn->name);
          }

        }
      }
    }
  }

  printf("exit_status: %d\n", (total_failure > 0) ? 1 : 0);
}