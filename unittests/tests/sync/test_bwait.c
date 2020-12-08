#include <stdint.h>

#include "lib.h"
#include "testlib.h"

typedef struct {
  bar_t* bar;
  uint64_t sz;
} bar_test_t;

void test_bwaits_cpu(int cpu, void* arg) {
  bar_test_t* test = arg;
  if (cpu < test->sz)
    BWAIT(cpu, test->bar, test->sz);
}

UNIT_TEST(test_bwaits_nodeadlock)
void test_bwaits_nodeadlock(void) {
  bar_t bar = EMPTY_BAR;

  for (int i = 0; i < 1000UL; i++) {
    bar_test_t test = {
      .bar = &bar,
      .sz = randrange(0,5),
    };
    run_on_cpus((async_fn_t*)test_bwaits_cpu, (void*)&test);
  }

  ASSERT(1);  /* assert we reach the end */
}