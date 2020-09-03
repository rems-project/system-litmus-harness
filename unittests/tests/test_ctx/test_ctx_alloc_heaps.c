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