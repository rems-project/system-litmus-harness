#include <stdint.h>

#include "lib.h"

lock_t __valloc_lock;

/* the actual memory pool
 */
valloc_mempool mem;

void init_valloc(void) {
  mem = (valloc_mempool){
    .top = TOP_OF_HEAP,
    .freelist = NULL,
    .chunks = { { 0, 0, NULL, NULL} },
    .chunk_alloc_list = NULL,
    .chunk_unalloc_list = NULL
  };

  /* we fill chunk_unalloc_list */
  for (uint64_t i = 0; i < NUM_ALLOC_CHUNKS; i++) {
    if (i == 0)
      mem.chunks[i].prev = NULL;
    else
      mem.chunks[i].prev = &mem.chunks[i-1];

    if (i == NUM_ALLOC_CHUNKS - 1)
      mem.chunks[i].next = NULL;
    else
      mem.chunks[i].next = &mem.chunks[i+1];
  }

  mem.chunk_unalloc_list = &mem.chunks[0];
}

void* realloc(void* p, uint64_t new_size) {
  char* new_p = alloc(new_size);
  valloc_alloc_chunk* chk = valloc_alloclist_find_alloc_chunk(&mem, (uint64_t)p);
  valloc_memcpy(new_p, p, chk->size);
  free(p);
  return new_p;
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

  valloc_free_chunk* free_chunk = valloc_freelist_find_best(size, alignment);
  if (free_chunk != NULL) {
    free_chunk = valloc_freelist_split_alignment(free_chunk, size, alignment);
    valloc_alloclist_alloc(&mem, (uint64_t)free_chunk, size);
    valloc_freelist_remove_chunk(free_chunk);
    unlock(&__valloc_lock);
    return free_chunk;
  }

  if (size > mem.top - BOT_OF_HEAP) {
    fail("! error: cannot allocate %p bytes, only %p bytes left to allocate\n", size, mem.top - BOT_OF_HEAP);
  }

  /* move 'top' down and align to size */
  uint64_t allocated_space_vaddr = ALIGN_POW2(mem.top - size, alignment);
  uint64_t new_top = allocated_space_vaddr;

  if (new_top < BOT_OF_HEAP) {
    puts("!! alloc_with_alignment: no free space\n");
    abort();
  }

  mem.top = new_top;
  valloc_alloclist_alloc(&mem, allocated_space_vaddr, size);
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
  valloc_alloc_chunk* chk = valloc_alloclist_find_alloc_chunk(&mem, (uint64_t)p);
  if (! chk) {
    fail("! err: free %p (double free?)\n", p);
  }
  uint64_t size = chk->size;

  valloc_alloclist_dealloc(&mem, (uint64_t)p);
  valloc_freelist_allocate_free_chunk((uint64_t)p, size);
  valloc_freelist_compact_chunk(mem.freelist);
  unlock(&__valloc_lock);
}

void free_all(void) {
  init_valloc(); /* can just re-init the mem struct to get back all memory */
}

static void __zero_all(void* p, uint64_t size) {
  char* ptr = (char* )p;

  uint64_t step = DCZVA_BLOCK_WIDTH;

  while (! IS_ALIGNED_TO((uint64_t)ptr, step)) {
    if (size <= 0)
      return;

    *ptr = 0;
    size--;
    ptr++;
  }

  while (size >= step) {
    asm volatile (
      "dc zva, %[loc]"
    :
    : [loc] "r" (ptr)
    : "memory"
    );
    size -= step;
    ptr += step;
  }

  for (; size > 0; size--, ptr++) {
    *ptr = 0;
  }
}

void valloc_memset(void* p, uint64_t value, uint64_t size) {
  if (value == 0 && DCZVA_ALLOW && MMU_ON && 0) {
    __zero_all(p, size);
    return;
  }

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

void memset(void* p, int val, size_t n) {
  valloc_memset(p, (uint64_t)val, (uint64_t)n);
}

uint64_t valloc_free_size(void) {
  uint64_t top = mem.top;

  uint64_t used_space = 0;
  valloc_alloc_chunk* chk = mem.chunk_alloc_list;
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

void memcpy(void* dest, void* src, size_t size) {
  valloc_memcpy(dest, src, (uint64_t)size);
}