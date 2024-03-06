
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
  for (u64 i = 0; i < NUM_ALLOC_CHUNKS; i++) {
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

void* realloc(void* p, u64 new_size) {
  char* new_p = alloc(new_size);
  valloc_alloc_chunk* chk = valloc_alloclist_find_alloc_chunk(&mem, (u64)p);
  valloc_memcpy(new_p, p, chk->size);
  free(p);
  return new_p;
}

static int is_pow2(u64 i) {
  while (i > 0) {
    if ((i % 2) == 1) {
      return (i == 1);
    }

    i >>= 1;
  }

  return 0;
}

static u64 next_largest_pow2(u64 i) {
  int hi=0;
  while (i > 0) {
    hi++;
    i >>= 1;
  }

  return (1UL << hi);
}

static void* __alloc_with_alignment(u64 size, u64 alignment) {
  valloc_free_chunk* free_chunk = valloc_freelist_find_best(size, alignment);
  if (free_chunk != NULL) {
    DEBUG(DEBUG_ALLOC_META, "for alloc size=%ld with alignment=0x%lx,best free chunk = %p\n", size, alignment, free_chunk);
    free_chunk = valloc_freelist_split_alignment(free_chunk, size, alignment);
    valloc_alloclist_alloc(&mem, (u64)free_chunk, size);
    valloc_freelist_remove_chunk(free_chunk);
    return free_chunk;
  }

  if (size > mem.top - BOT_OF_HEAP) {
    fail("! error: cannot allocate %p bytes, only %p bytes left to allocate\n", size, mem.top - BOT_OF_HEAP);
  }

  /* move 'top' down and align to size */
  u64 allocated_space_vaddr = ALIGN_POW2(mem.top - size, alignment);
  u64 new_top = allocated_space_vaddr;

  if (new_top < BOT_OF_HEAP) {
    puts("!! alloc_with_alignment: no free space\n");
    abort();
  }

  u64 allocated_size = mem.top - new_top;
  mem.top = new_top;
  valloc_alloclist_alloc(&mem, allocated_space_vaddr, allocated_size);

  return (void*)allocated_space_vaddr;
}

void* alloc_with_alignment(u64 size, u64 alignment) {
  LOCK(&__valloc_lock);

  /* minimum allocation */
  if (size < sizeof(valloc_free_chunk)) {
    size = next_largest_pow2(sizeof(valloc_free_chunk));
    alignment = 8;
  }

  /* minimum alignment */
  if (alignment < sizeof(valloc_alloc_chunk)) {
    alignment = next_largest_pow2(sizeof(valloc_alloc_chunk));
  }

  void* ptr = __alloc_with_alignment(size, alignment);

  /* always zero */
  valloc_memset(ptr, 0, size);

  UNLOCK(&__valloc_lock);
  return ptr;
}

void* alloc(u64 size) {
  u64 alignment;

  if (! is_pow2(size))
    size = next_largest_pow2(size);

  /* no point aligning on anything bigger than a 64-bit pointer
   * for more explicit alignments use alloc_with_alignment
   */
  alignment = size <= 8 ? size : 8;

  /* minimum allocation */
  if (size < sizeof(valloc_free_chunk)) {
    size = next_largest_pow2(sizeof(valloc_free_chunk));
    alignment = 8;
  }

  LOCK(&__valloc_lock);
  void* ptr = __alloc_with_alignment(size, alignment);

  /* always zero */
  valloc_memset(ptr, 0, size);

  UNLOCK(&__valloc_lock);
  return ptr;
}

void free(void* p) {
  LOCK(&__valloc_lock);
  valloc_alloc_chunk* chk = valloc_alloclist_find_alloc_chunk(&mem, (u64)p);
  if (! chk) {
    fail("! err: free %p (double free?)\n", p);
  }
  u64 size = chk->size;

  valloc_alloclist_dealloc(&mem, (u64)p);
  valloc_freelist_allocate_free_chunk((u64)p, size);
  valloc_freelist_compact_chunk(mem.freelist);
  UNLOCK(&__valloc_lock);
}

void free_all(void) {
  init_valloc(); /* can just re-init the mem struct to get back all memory */
}

static void __zero_all(void* p, u64 size) {
  char* ptr = (char* )p;

  u64 step = DCZVA_BLOCK_WIDTH;

  while (! IS_ALIGNED_TO((u64)ptr, step)) {
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

void valloc_memset(void* p, u8 value, u64 size) {
  if (value == 0 && DCZVA_ALLOW && MMU_ON && 0) {
    __zero_all(p, size);
    return;
  }

  char* ptr = p;
  u64 end = (u64)ptr + size;
  for (; (u64)ptr < end; ptr++) {
    *ptr = value;
  }
}

int valloc_is_free(void* p) {
  u64 va = (u64)p;

  if (va < mem.top) {
    return 1;
  }

  valloc_free_chunk* fblk = mem.freelist;
  while (fblk != NULL) {
    u64 fblk_start = (u64)fblk;
    u64 fblk_end = fblk_start + fblk->size;
    if (fblk_start <= va && va <= fblk_end) {
      return 1;
    }
    fblk = fblk->next;
  }

  return 0;
}

void memset(void* p, int val, size_t n) {
  valloc_memset(p, (u64)val, (u64)n);
}

u64 valloc_free_size(void) {
  u64 top = mem.top;

  u64 used_space = 0;
  valloc_alloc_chunk* chk = mem.chunk_alloc_list;
  while (chk != NULL) {
    used_space += chk->size;
    chk = chk->next;
  }

  return (top - BOT_OF_HEAP) - used_space;
}

void valloc_memcpy(void* dest, void* src, u64 size) {
  char* p = (char*)src;
  char* q = (char*)dest;
  for (u64 i = 0; i < size; i++) {
    *(q + i) = *(p + i);
  }
}

void memcpy(void* dest, void* src, size_t size) {
  valloc_memcpy(dest, src, (u64)size);
}


/* debugging functions */
u64 valloc_alloclist_count_chunks(void) {
  valloc_mempool* pool = &mem;

  u64 counter = 0;
  valloc_alloc_chunk* chunk = pool->chunk_alloc_list;
  while (chunk != NULL) {
    counter += 1;
    chunk = chunk->next;
  }

  return counter;
}