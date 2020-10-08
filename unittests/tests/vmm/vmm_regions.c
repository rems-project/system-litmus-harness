#include <stdint.h>

#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_vmm_region_testdata_round_trip)
void test_vmm_region_testdata_round_trip(void) {

  for (int i = 0; i < 10; i++) {
    uint64_t pa = randrange(TESTDATA_MMAP_BASE, TESTDATA_MMAP_BASE+TESTDATA_MMAP_SIZE);

    uint64_t va = TESTDATA_MMAP_PHYS_TO_VIRT(pa);
    uint64_t pa2 = TESTDATA_MMAP_VIRT_TO_PHYS(va);

    ASSERT(pa2 == pa, "pa2 (%p) is not pa (%p) via va (%p)", pa2, pa, va);
  }
}