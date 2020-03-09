#include <stdint.h>

#include "lib.h"

void init_valloc(void) {
  mem = (valloc_mempool){
      .top = TOP_OF_MEM,
      .space_free = TOTAL_HEAP,
      .freelist = NULL,
  };
}

char* __free_check(uint64_t size) {
  valloc_alloc** last = (valloc_alloc**)&mem.freelist;
  valloc_alloc* free = mem.freelist;
  while (free) {
    if (free->size >= size) {
      *last = free->next;
      return (char*)free;
    }

    last = &free->next;
    free = free->next;
  }
  return 0;
}

void* alloc_with_alignment(uint64_t size, uint64_t pow2_alignment) {
  /* first: check if there space in freelist */
  char* p = __free_check(size);
  if (p) return p;

  uint64_t allocated_space_vaddr = ALIGN_POW2(mem.top - size, pow2_alignment);
  uint64_t blk_addr = (allocated_space_vaddr - sizeof(valloc_block));
  *(valloc_block*)blk_addr = (valloc_block){
    .size = size,
  };

  uint64_t allocated_space = mem.top - allocated_space_vaddr;
  uint64_t used_space = allocated_space + sizeof(valloc_block);

  mem.space_free = mem.space_free - used_space;
  uint64_t new_top = blk_addr;

  if (new_top < BOT_OF_HEAP) {
    puts("!! alloc_with_alignment: no free space\n");
    abort();
  }

  mem.top = new_top;
  return (void*)allocated_space_vaddr;
}

static int is_pow2(uint64_t i) {
  while (i > 0) {
    if ((i % 2) == 1) {
      return (i == 1);
    }

    i >>= 1;
  }

  return 0;
}

static uint64_t nearest_pow2(uint64_t i) {
  int hi=0;
  while (i > 0) {
    hi++;
    i >>= 1;
  }

  return (1UL << hi);
}

void* alloc(uint64_t size) {
  if (is_pow2(size)) {
    return alloc_with_alignment(size, size);
  } else {
    return alloc_with_alignment(size, nearest_pow2(size));
  }
}

void __find_and_remove_from_freelist(char* p) {
  uint64_t va = (uint64_t)p;

  valloc_alloc* fblk = mem.freelist;
  valloc_alloc** prev = &mem.freelist;

  while (fblk != NULL) {
    if ((uint64_t)fblk == va) {
      *prev = fblk->next;
      return;
    }
    prev = &fblk->next;
    fblk = fblk->next;
  }
}

int __compact_unallocated(uint64_t va_start, uint64_t va_end) {
  valloc_alloc* fblk = mem.freelist;

  while (fblk != NULL) {
    uint64_t f_start = (uint64_t)fblk;
    uint64_t f_end = f_start + fblk->size;
    fblk = fblk->next;

    if ((uint64_t)fblk < va_start)
      continue;

    if (va_end < f_start)
      continue;

    if ((uint64_t)fblk < va_end) {
      return 0;
    }
  }

  return 1;
}

void __compact_node(valloc_alloc* node) {
  uint64_t begin = (uint64_t)node;

  /**
   *  find fblk such that fblk end == node start
   * 
   *  n1 -> node -> n2 -> ...
   *  n3 -> fblk -> n4 -> ...
   * 
   * refactor into:
   *  n1 -> fblk+node -> n2
   *  n3 -> n4
   */

  valloc_alloc* fblk = mem.freelist;
  while (fblk != NULL) {
    uint64_t fblk_end = (uint64_t)fblk + fblk->size;
    if (fblk_end < begin && __compact_unallocated(fblk_end+1, begin-1)) {
      node->size = ((uint64_t)fblk + fblk->size) - (uint64_t)node;
      __find_and_remove_from_freelist(fblk);
      return;
    }
    fblk = fblk->next;
  }
}

void compact(void) {
  valloc_alloc* fblk = mem.freelist;

  while (fblk != NULL) {
    __compact_node(fblk);
    fblk = fblk->next;
  }
}


void free(void* p) {
  valloc_block* blk = (valloc_block*)((uint64_t)p - sizeof(valloc_block));
  uint64_t size = blk->size;

  *(valloc_alloc*)p = (valloc_alloc){
      .size = size,
      .next = mem.freelist,
  };

  mem.freelist = (valloc_alloc*)p;
  compact();
}

void free_all(void) {
  init_valloc(); /* can just re-init the mem struct to get back all memory */
}

void valloc_memset(void* p, uint64_t value, uint64_t size) {
  char* ptr = p;
  uint64_t end = (uint64_t)ptr + size;
  for (; (uint64_t)ptr < end; ptr++) {
    *ptr = value;
  }
}

int valloc_is_free(void* p) {
  uint64_t va = (uint64_t)p;

  if (va < mem.top) {
    return 1;
  }

  valloc_alloc* fblk = mem.freelist;
  while (fblk != NULL) {
    if ((uint64_t)fblk < va && va < ((uint64_t)fblk) + fblk->size) {
      return 1;
    }
    fblk = fblk->next;
  }

  return 0;
}