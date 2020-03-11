#include <stdint.h>

#include "lib.h"

/* debug() defined in printer.c */

/* valloc debug */

int __free_idx(uint64_t va) {
  int idx = 0;

  if (va < mem.top) {
    return -1;
  }

  valloc_alloc* fblk = mem.freelist;
  while (fblk != NULL) {
    uint64_t fblk_start = (uint64_t)fblk - sizeof(valloc_block);
    uint64_t fblk_end = (uint64_t)fblk + fblk->size;
    if (fblk_start <= va && va <= fblk_end) {
      return idx;
    }
    fblk = fblk->next;
    idx++;
  }

  return -1;
}


void __print_heap(void) {
  for (uint64_t i = mem.top; i < TOP_OF_MEM; i++) {
    if (valloc_is_free((void*)i)) {
      int idx = __free_idx((uint64_t)i);
      printf("[%d]",idx);
    } else {
      printf("[x]");
    }
  }
  printf("\n");
}

void __print_freelist(void) {
  valloc_alloc* fblk = mem.freelist;
  while (fblk != NULL) {
    printf("[%p -> %p] -> ", (uint64_t)fblk-sizeof(valloc_block), (uint64_t)fblk+fblk->size);
    fblk = fblk->next;
  }
  printf("\n");
}


void debug_show_valloc_mem(void)  {
  printf("* HEAP:\n");
  __print_heap();
  printf("**FREELIST: ");
  __print_freelist();
}


/* vmm debug */

void debug_vmm_show_walk(uint64_t* pgtable, uint64_t va){
  uint64_t* root = pgtable;
  for (int level = 0; level <= 3; level++) {
    uint64_t* p = root + OFFS(va, level);
    desc_t desc = read_desc(*p, level);
    printf("  @ level %d : (OFFS = %d) @ %p => ", level, OFFS(va, level), p);
    show_desc(desc);
    if (desc.type == Invalid || desc.type == Block) {
      printf("  (end)\n");
      return;
    }

    root = (uint64_t*)desc.table_addr;
  }
}
