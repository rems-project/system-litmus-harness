
#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_desc_read)
void test_desc_read(void) {
  u64 pgd = 0x40080703UL;
  desc_t desc = read_desc(pgd, 3);
  ASSERT(desc.oa == 0x40080000UL, "oa");
  ASSERT(desc.type == Block, "type");
  ASSERT(desc.attrs.AF == 1, "AF");
  ASSERT(desc.attrs.SH == 3, "SH");
  ASSERT(desc.attrs.AP == 0, "AP");
}

UNIT_TEST(test_desc_roundtrip)
void test_desc_roundtrip(void) {
  u64 pgd = 0x40080703UL;
  desc_t desc = read_desc(pgd, 3);
  ASSERT(write_desc(desc) == pgd, "roundtrip");
}