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

uint64_t __total_alloc(void) {
  uint64_t s = 0;
  valloc_alloc_chunk* next = mem.chunk_alloc_list;
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
    uint64_t total_alloc = __total_alloc();
    printf("(valloc)  top=0x%lx, #total_used=0x%lx B #free_chunks=0x%lx (with %p B)\n", mem.top, total_alloc, free_chunks, free_mem);
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

/** for a 12-bit aligned page VA *pg*
 * print out a line like:
 *
 * 0xPGADDR000: [0xPGADDR123: 32, 0xPGADDR456: 72]
 *
 * this indicates that in the page 0xPGADDR000 has 2 chunks of allocated space in it
 *  the first starting at offset 0x123, with 32-bytes of allocation
 *  then the next at offset 0x456 with 72 bytes.
 */

/** buffer in which to store the time information
 * from the alloc chunk's metadata
 *
 * this is unprotected, since __print_heap_page does not take locks
 */
char __debug_print_heap_page_time_buf[64];

void __print_heap_page(uint64_t pg) {
  uint64_t total_alloc = 0;
  printf("%p: \n");

  int va = pg + 0;

  while (va < pg + 4096) {
    valloc_alloc_chunk* chk = valloc_alloclist_find_alloc_chunk(&mem, va);

    if (chk == NULL) {
      va += 1;
    } else {
#if DEBUG_ALLOC_META
      char* time_str = __debug_print_heap_page_time_buf;
      valloc_alloc_chunk_debug_metadata* meta = &chk->meta;
      sprint_time(&time_str[0], meta->ts, SPRINT_TIME_HHMMSSCLK);

      printf("   ");

      if (chk->start < pg) {
        printf("...");
      }

      printf("[%p: %p @ %s from %p]\n", chk->start, chk->size, &time_str[0], meta->where);
#else
      printf("[%p: %p]\n", chk->start, chk->size);
#endif
      va += chk->size;
      total_alloc += chk->size;

      if (chk->start < pg) {
        total_alloc -= (pg - chk->start);
      }
    }
  }

  printf("   (%ld/4096 B allocated) \n", total_alloc);
}

void __print_heap_pages(void) {
  for (uint64_t i = ALIGN_TO(TOP_OF_HEAP, 12); i >= ALIGN_TO(mem.top, 12); i -= 4096UL) {
    __print_heap_page(i);
  }
}

void __print_freelist(void) {
  valloc_free_chunk* fblk = mem.freelist;
  printf("[%p]\n", fblk);
  while (fblk != NULL) {
    printf(" [%p -> %p (%ld B)]\n", (uint64_t)fblk, (uint64_t)fblk+fblk->size, fblk->size);
    fblk = fblk->next;
  }
  printf("[end of freelist]\n");
  printf("\n");
}


void debug_show_valloc_mem(void)  {
  printf("* HEAP:\n");
  __print_heap_pages();
  printf("**FREELIST:\n");
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