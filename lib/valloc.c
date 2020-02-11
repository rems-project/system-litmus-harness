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
  valloc_alloc** last = &mem.freelist;
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

  uint64_t new_top = ALIGN_POW2(mem.top - size, pow2_alignment);
  mem.top = new_top;
  mem.space_free = mem.space_free - size;

  if (new_top < BOT_OF_HEAP) {
    puts("!! alloc_with_alignment: no free space\n");
    abort();
  }

  return (void*)new_top;
}

void* alloc(uint64_t size) { return alloc_with_alignment(size, 1); }

void* alloc_aligned(uint64_t pow2_size) {
  return alloc_with_alignment(pow2_size, pow2_size);
}

void free(char* p, uint64_t size) {
  *(valloc_alloc*)p = (valloc_alloc){
      .size = size,
      .next = mem.freelist,
  };
  mem.freelist = (valloc_alloc*)p;
}

void free_all(void) {
  init_valloc(); /* can just re-init the mem struct to get back all memory */
}

void valloc_memset(char* p, uint64_t value, uint64_t size) {
  uint64_t end = (uint64_t)p + size;
  for (; (uint64_t)p < end; p++) {
    *p = value;
  }
}