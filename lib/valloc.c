#include <stdint.h>

#include "lib.h"

void init_valloc(void) {
  mem = (valloc_mempool){
      .top = TOP_OF_MEM,
      .space_free = TOTAL_HEAP,
      .freelist = 0,
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
  uint64_t inp = i;
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


void free(void* p) {
  valloc_block* blk = (valloc_block*)((uint64_t)p) - sizeof(valloc_block);
  uint64_t size = blk->size;

  *(valloc_alloc*)p = (valloc_alloc){
      .size = size,
      .next = mem.freelist,
  };

  mem.freelist = (valloc_alloc*)p;
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