#include "lib.h"

void initialize_regions(regions_t* r) {
  /* the test data (variables etc) are placed into
   * physical address space TOP_OF_HEAP -> TOP_OF_MEM
   *
   * we allocate 16 x 8M contiguous regions
   * but spread evenly throughout the 128G VA space
   *
   * see vmm_pgtable.c
   */

  for (int i = 0 ; i < NR_REGIONS; i++) {
    uint64_t start_va = TESTDATA_MMAP_8M_VA_FROM_INDEX(i);
    debug("r->regions[%d] = %p\n", i, start_va);
    r->regions[i] = (region_t*)start_va;
  }
}


region_idx_t align_down_region_idx(region_idx_t idx, pin_level_t alignment) {
  return (region_idx_t){.reg_ix=idx.reg_offs, .reg_offs=ALIGN_TO(idx.reg_offs, LEVEL_SHIFTS[alignment])};
}

region_idx_t align_up_region_idx(region_idx_t idx, pin_level_t alignment) {
  return (region_idx_t){.reg_ix=idx.reg_offs, .reg_offs=ALIGN_UP_TO(idx.reg_offs, LEVEL_SHIFTS[alignment])};
}

uint64_t va_from_region_idx(test_ctx_t* ctx, region_idx_t idx) {
  uint64_t reg_start = (uint64_t)ctx->heap_memory.regions[idx.reg_ix];
  return reg_start + idx.reg_offs;
}

region_idx_t region_idx_bottom() {
  return (region_idx_t){.reg_ix=0, .reg_offs=0};
}

region_idx_t region_idx_top() {
  return (region_idx_t){.reg_ix=NR_REGIONS-1, .reg_offs=BITMASK(REGION_SHIFT)};
}

inline char* region_idx_to_str(region_idx_t ix) {
  char* s = ALLOC_MANY(char, 1024);
  sprintf(s, "(region_idx){.reg_ix=%lx, .reg_offs=%lx}", ix.reg_ix, ix.reg_offs);
  return s;
}