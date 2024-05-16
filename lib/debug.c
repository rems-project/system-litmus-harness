
#include "lib.h"

/* debug() defined in printer.c */

/* valloc debug */

u64 __count_free_chunks(void) {
  u64 s = 0;
  valloc_free_chunk* next = mem.freelist;
  while (next != NULL) {
    s++;
    next = next->next;
  }
  return s;
}

u64 __total_free(void) {
  u64 s = 0;
  valloc_free_chunk* next = mem.freelist;
  while (next != NULL) {
    s += next->size;
    next = next->next;
  }
  return s;
}

u64 __total_alloc(void) {
  u64 s = 0;
  valloc_alloc_chunk* next = mem.chunk_alloc_list;
  while (next != NULL) {
    s += next->size;
    next = next->next;
  }
  return s;
}

u64 __remaining_unalloc_chunks(void) {
  u64 c = 0;
  valloc_alloc_chunk* next = mem.chunk_unalloc_list;
  while (next != NULL) {
    c++;
    next = next->next;
  }
  return c;
}

void debug_valloc_status(void) {
  if (DEBUG) {
    u64 free_chunks = __count_free_chunks();
    u64 free_mem = __total_free();
    u64 unalloc_count = __remaining_unalloc_chunks();
    u64 total_alloc = __total_alloc();
    printf(
      "(valloc)  top=0x%lx, #total_used=0x%lx B #remaining_chunks=%ld #free_chunks=0x%lx (with %p B)\n",
      mem.top,
      total_alloc,
      unalloc_count,
      free_chunks,
      free_mem
    );
  }
}

int __free_idx(u64 va) {
  int idx = 0;

  if (va < mem.top) {
    return -1;
  }

  valloc_free_chunk* fblk = mem.freelist;
  while (fblk != NULL) {
    u64 fblk_start = (u64)fblk;
    u64 fblk_end = fblk_start + fblk->size;
    if (fblk_start <= va && va <= fblk_end) {
      return idx;
    }
    fblk = fblk->next;
    idx++;
  }

  return -1;
}

void __print_heap_flat(void) {
  for (u64 i = mem.top; i < TOP_OF_MEM; i++) {
    if (valloc_is_free((void*)i)) {
      int idx = __free_idx((u64)i);
      printf("[%d]", idx);
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

void __print_heap_page(u64 pg) {
  u64 total_alloc = 0;
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
      sprint_time(NEW_BUFFER(time_str, 1024), meta->ts, SPRINT_TIME_HHMMSSCLK);

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
  for (u64 i = ALIGN_TO(TOP_OF_HEAP, 12); i >= ALIGN_TO(mem.top, 12); i -= 4096UL) {
    __print_heap_page(i);
  }
}

void __print_freelist(void) {
  valloc_free_chunk* fblk = mem.freelist;
  printf("[%p]\n", fblk);
  while (fblk != NULL) {
    printf(" [%p -> %p (%ld B)]\n", (u64)fblk, (u64)fblk + fblk->size, fblk->size);
    fblk = fblk->next;
  }
  printf("[end of freelist]\n");
  printf("\n");
}

void debug_show_valloc_mem(void) {
  printf("* HEAP:\n");
  __print_heap_pages();
  printf("**FREELIST:\n");
  __print_freelist();
}

/* vmm debug */

void debug_vmm_show_walk(u64* pgtable, u64 va) {
  u64* root = pgtable;
  for (int level = 0; level <= 3; level++) {
    u64* p = root + OFFS(va, level);
    desc_t desc = read_desc(*p, level);
    printf("  @ level %d : (OFFS = %d) @ %p => ", level, OFFS(va, level), p);
    show_desc(desc);
    if (desc.type == Invalid || desc.type == Block) {
      printf("  (end)\n");
      return;
    }

    root = (u64*)desc.table_addr;
  }
}

const char hex[] = "0123456789abcdef";

void dump_hex(char* dest, char* src, int len) {
  int cur = 0;
  dest[cur++] = '\'';
  for (int i = 0; i < len; i += 1) {
    u8 c = *(src + i);
    u8 hi = (c & 0xf0) >> 4;
    u8 lo = (c & 0x0f);

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