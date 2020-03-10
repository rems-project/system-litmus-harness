#include <stdint.h>

#include "lib.h"
#include "testlib.h"

UNIT_TEST(test_valloc_free_save)
void test_valloc_free_save(void) {
  char* p = alloc(64);
  uint64_t sz = *(p - 8);
  free(p);
  ASSERT(sz >= 64);
}

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
  ASSERT(mem.freelist == NULL, "free not NULL");
  char* p = alloc(64);
  char* q = alloc(64);
  free(p);
  /* intentionally no free q */
  ASSERT(mem.freelist != NULL, "free still NULL");
  ASSERT(mem.freelist->size >= 64, "free size too small");
}

UNIT_TEST(test_valloc_free_compact)
void test_valloc_free_compact(void) {
  char* p = alloc(64);
  char* q = alloc(64);
  free(p);
  free(q);

  ASSERT(mem.freelist == NULL, "non-null freelist");
  ASSERT(mem.top == TOP_OF_MEM, "mem reset");
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
  uint64_t* p = vmm_alloc_new_idmap_4k();
  vmm_free_pgtable(p);
  ASSERT(space == valloc_free_size());
}