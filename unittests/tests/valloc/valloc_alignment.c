
#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_valloc_alloc_aligned)
void test_valloc_alloc_aligned(void) {
  /* using just alloc() the argument might not be fully-aligned */
  char* p = alloc(1024);
  FREE(p);
  ASSERT(IS_ALIGNED((u64)p, 3));
}

UNIT_TEST(test_valloc_alloc_with_alignment_is_aligned)
void test_valloc_alloc_with_alignment_is_aligned(void) {
  /* using just alloc() the argument might not be aligned */
  char* p = alloc_with_alignment(1024, 1024);
  FREE(p);
  ASSERT(IS_ALIGNED((u64)p, 10));
}

UNIT_TEST(test_valloc_alloc_minimal_alignment)
void test_valloc_alloc_minimal_alignment(void) {
  /* using just alloc() the argument might not be aligned */
  char* p = alloc(1);
  FREE(p);
  /* but it should have at least a minimum alignment of 64-bits */
  ASSERT(IS_ALIGNED((u64)p, 3));
}