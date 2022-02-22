
#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_vmm_region_testdata_round_trip)
void test_vmm_region_testdata_round_trip(void) {

  for (int i = 0; i < 10; i++) {
    u64 pa = randrange(TESTDATA_MMAP_BASE, TESTDATA_MMAP_BASE+TESTDATA_MMAP_SIZE);

    u64 va = TESTDATA_MMAP_PHYS_TO_VIRT(pa);
    u64 pa2 = TESTDATA_MMAP_VIRT_TO_PHYS(va);

    ASSERT(pa2 == pa, "pa2 (%p) is not pa (%p) via va (%p)", pa2, pa, va);
  }
}