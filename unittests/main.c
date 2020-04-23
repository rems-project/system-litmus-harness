#include "lib.h"
#include "testlib.h"

#include "tests.externs"

test_file_t* tests;


void main(void) {
  unsigned int total_count = 0;
  unsigned int total_failure = 0;

  tests = (test_file_t[]){
      #include "tests.cstruct"
  };

  for (int fidx = 0; fidx < NO_TEST_FILES; fidx++) {
    test_file_t* f = &tests[fidx];
    printf("--- %s ---\n", f->name);

    for (int tidx = 0; tidx < f->no_tests; tidx++) {
      /* horrible hack to "reset" memory allocations back to where it was before
       */
      valloc_mempool pool = mem;
      unit_test_t* fn = &f->fns[tidx];
      current_test = fn;
      fn->fn();
      if (ENABLE_PGTABLE) {
        vmm_switch_ttable(vmm_pgtable);
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
    }

    printf("\n");
  }

  printf("total: %d failures of %d runs\n", total_failure, total_count);

  if (total_failure > 0) {
    printf("Failures: \n");
    for (int fidx = 0; fidx < NO_TEST_FILES; fidx++) {
      test_file_t* f = &tests[fidx];
      for (int tidx = 0; tidx < f->no_tests; tidx++) {
        unit_test_t* fn = &f->fns[tidx];
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
}