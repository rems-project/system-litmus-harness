#include <stdint.h>

#include "lib.h"

lock_t __valloc_lock;

valloc_alloc_chunk chunks[NUM_ALLOC_CHUNKS];
valloc_alloc_chunk* chunk_unalloc_list;
valloc_alloc_chunk* chunk_alloc_list;

void init_valloc(void) {
  mem = (valloc_mempool){
      .top = TOP_OF_MEM,
      .freelist = NULL,
  };

  for (uint64_t i = 0; i < NUM_ALLOC_CHUNKS; i++) {
    if (i == 0)
      chunks[i].prev = NULL;
    else
      chunks[i].prev = &chunks[i-1];

    if (i == NUM_ALLOC_CHUNKS - 1)
      chunks[i].next = NULL;
    else
      chunks[i].next = &chunks[i+1];
  }

  chunk_unalloc_list = &chunks[0];
  chunk_alloc_list = NULL;
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

void* alloc_with_alignment(uint64_t size, uint64_t alignment) {
  /* minimum allocation */
  if (size < sizeof(valloc_free_chunk)) {
    size = nearest_pow2(sizeof(valloc_free_chunk));
    alignment = size;
  }

  lock(&__valloc_lock);
  debug("alloc %p (align %p)\n", size, alignment);
  valloc_free_chunk* free_chunk = valloc_freelist_find_best(size, alignment);
  if (free_chunk != NULL) {
    free_chunk = valloc_freelist_split_alignment(free_chunk, size, alignment);
    valloc_alloclist_alloc((uint64_t)free_chunk, size);
    valloc_freelist_remove_chunk(free_chunk);
    unlock(&__valloc_lock);
    return free_chunk;
  }

  /* move 'top' down and align to size */
  uint64_t allocated_space_vaddr = ALIGN_POW2(mem.top - size, alignment);
  uint64_t new_top = allocated_space_vaddr;

  if (new_top < BOT_OF_HEAP) {
    puts("!! alloc_with_alignment: no free space\n");
    abort();
  }

  mem.top = new_top;
  valloc_alloclist_alloc(allocated_space_vaddr, size);
  unlock(&__valloc_lock);
  return (void*)allocated_space_vaddr;
}

void* alloc(uint64_t size) {
  if (! is_pow2(size))
    size = nearest_pow2(size);

  /* no point aligning on anything bigger than a whole page */
  uint64_t alignment = size <= 4096 ? size : 4096;
  return alloc_with_alignment(size, alignment);
}

void free(void* p) {
  lock(&__valloc_lock);
  valloc_alloc_chunk* chk = valloc_alloclist_find_alloc_chunk((uint64_t)p);
  if (! chk) {
    fail("! err: free %p (double free?)\n", p);
  }
  uint64_t size = chk->size;

  valloc_alloclist_dealloc((uint64_t)p);
  valloc_freelist_allocate_free_chunk((uint64_t)p, size);
  valloc_freelist_compact_chunk(mem.freelist);
  unlock(&__valloc_lock);
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

  valloc_free_chunk* fblk = mem.freelist;
  while (fblk != NULL) {
    uint64_t fblk_start = (uint64_t)fblk;
    uint64_t fblk_end = fblk_start + fblk->size;
    if (fblk_start <= va && va <= fblk_end) {
      return 1;
    }
    fblk = fblk->next;
  }

  return 0;
}

uint64_t valloc_free_size(void) {
  uint64_t top = mem.top;

  uint64_t used_space = 0;
  valloc_alloc_chunk* chk = chunk_alloc_list;
  while (chk != NULL) {
    used_space += chk->size;
    chk = chk->next;
  }

  return (top - BOT_OF_HEAP) - used_space;
}

void valloc_memcpy(void* dest, void* src, uint64_t size) {
  char* p = (char*)src;
  char* q = (char*)dest;
  for (uint64_t i = 0; i < size; i++) {
    *(q + i) = *(p + i);
  }
}