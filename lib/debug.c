#include <stdint.h>

#include "lib.h"

/* debug() defined in printer.c */

/* valloc debug */

uint64_t __count_free_chunks(void) {
  uint64_t s = 0;
  valloc_free_chunk* next = mem.freelist;
  while (next != NULL) {
    s++;
    next = next->next;
  }
  return s;
}

uint64_t __total_free(void) {
  uint64_t s = 0;
  valloc_free_chunk* next = mem.freelist;
  while (next != NULL) {
    s += next->size;
    next = next->next;
  }
  return s;
}

void debug_valloc_status(void) {
  if (DEBUG) {
    uint64_t free_chunks = __count_free_chunks();
    uint64_t free_mem = __total_free();
    printf("(valloc)  top=0x%lx,  #free_chunks=0x%lx (with %p B)\n", mem.top, free_chunks, free_mem);
  }
}

int __free_idx(uint64_t va) {
  int idx = 0;

  if (va < mem.top) {
    return -1;
  }

  valloc_free_chunk* fblk = mem.freelist;
  while (fblk != NULL) {
    uint64_t fblk_start = (uint64_t)fblk;
    uint64_t fblk_end = fblk_start + fblk->size;
    if (fblk_start <= va && va <= fblk_end) {
      return idx;
    }
    fblk = fblk->next;
    idx++;
  }

  return -1;
}


void __print_heap_flat(void) {
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

void __print_heap_page(uint64_t pg) {
  uint64_t was_free = 1;
  uint64_t chunk = 0;
  uint64_t total_alloc = 0;
  printf("%p: ");
  for (uint64_t i = 0; i < 4096; i++) {
    if (pg+i > TOP_OF_MEM)
      break;

    uint64_t is_free = valloc_is_free((void*)(pg+i));
    if (! is_free) {
      total_alloc++;
    }

    if (is_free != was_free) {
      if (! was_free) {
        printf("[%p: %ld]", pg+i-chunk, chunk);
      }

      chunk = 0;
    }

    chunk++;
    was_free = is_free;
  }

  if (chunk != 0) {
    printf("[%p: %ld]", pg+1024-chunk, chunk);
  }

  printf(" (%ld B) \n", total_alloc);
}

void __print_heap_pages(void) {
  for (uint64_t i = ALIGN_TO(mem.top, 12); i < TOP_OF_MEM; i += 4096UL) {
    __print_heap_page(i);
  }
}

void __print_freelist(void) {
  valloc_free_chunk* fblk = mem.freelist;
  while (fblk != NULL) {
    printf("[%p -> %p] -> ", (uint64_t)fblk, (uint64_t)fblk+fblk->size);
    fblk = fblk->next;
  }
  printf("\n");
}


void debug_show_valloc_mem(void)  {
  printf("* HEAP:\n");
  __print_heap_pages();
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

const char hex[] = "0123456789abcdef";

void dump_hex(char* dest, char* src, int len) {
  int cur = 0;
  dest[cur++] = '\'';
  for (int i = 0; i < len; i += 1) {
    uint8_t c = *(src+i);
    uint8_t hi = (c & 0xf0) >> 4;
    uint8_t lo = (c & 0x0f);

    if ((32 <= c && c <= 90) || (97 <= c && c <= 122)) {
      dest[cur++] = c;
    } else {
      dest[cur++] = '\\';
      dest[cur++] = 'x';
      dest[cur++] = hex[hi];
      dest[cur++] = hex[lo];
    }
  }
  dest[cur++] = '\'';
  dest[cur++] = '\0';
}