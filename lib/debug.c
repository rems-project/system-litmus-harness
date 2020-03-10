#include <stdint.h>

#include "lib.h"

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