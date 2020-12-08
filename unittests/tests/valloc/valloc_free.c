#include <stdint.h>

#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_valloc_free_reuse)
void test_valloc_free_reuse(void) {
  char* p = alloc(64);
  free(p);
  char* q = alloc(64);
  free(q);
  ASSERT((uint64_t)p == (uint64_t)q);
}

UNIT_TEST(test_valloc_freelist)
void test_valloc_freelist(void) {
  char* p = alloc(64);
  char* q = alloc(64);
  free(p);
  /* intentionally no free q */

  ASSERT(mem.freelist != NULL, "free still NULL");
  ASSERT(mem.freelist->size >= 64, "free size too small");

  /* use q to suppress warnings
   * this free is not strictly necessary as the tester should cleanup
   */
  free(q);
}

UNIT_TEST(test_valloc_free_compact_reuse)
void test_valloc_free_compact_reuse(void) {
  int h = valloc_freelist_hash(mem.freelist);
  int t = mem.top;

  char* p = alloc(64);
  char* q = alloc(64);
  free(p);
  free(q);

  ASSERT(valloc_freelist_hash(mem.freelist) == h, "changed freelist");
  ASSERT(mem.top == t, "mem reset");
}

UNIT_TEST(test_valloc_free_compact_all)
void test_valloc_free_compact_all(void) {
  uint64_t space = valloc_free_size();
  char* p = alloc(64);
  free(p);
  ASSERT(space == valloc_free_size());
}

UNIT_TEST(test_valloc_free_pgtable)
void test_valloc_free_pgtable(void) {
  uint64_t space = valloc_free_size();
  uint64_t* p = vmm_alloc_new_4k_pgtable();
  vmm_free_generic_pgtable(p);
  ASSERT(space == valloc_free_size());
}