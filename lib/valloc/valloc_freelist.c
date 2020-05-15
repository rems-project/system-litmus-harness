#include <stdint.h>

#include "lib.h"

/*
 * thread-unsafe functions for modifying the freelist
 */

uint64_t valloc_freelist_chunk_size(valloc_free_chunk* chunk) {
  return chunk->size;
}

uint64_t valloc_freelist_start_of_chunk(valloc_free_chunk* chunk) {
  return (uint64_t)chunk;
}

void valloc_freelist_allocate_free_chunk(uint64_t addr, uint64_t size) {
  valloc_free_chunk* chunk = (valloc_free_chunk*)addr;
  chunk->size = size;

  SET(mem.freelist, prev, chunk);
  chunk->next = mem.freelist;
  chunk->prev = NULL;
  mem.freelist = chunk;
}

/**
 * locate the smallest free chunk that this allocation
 * could fit into
 */
valloc_free_chunk* valloc_freelist_find_best(uint64_t size, uint64_t alignment) {
  valloc_free_chunk* cur = mem.freelist;

  uint64_t smallest_space = ~0;
  valloc_free_chunk* best = NULL;

  while (cur != NULL) {
    uint64_t start_of_chunk = valloc_freelist_start_of_chunk(cur);
    uint64_t space_in_chunk = valloc_freelist_chunk_size(cur);
    uint64_t align = ALIGN_UP_TO(start_of_chunk, alignment) - start_of_chunk;
    if ((space_in_chunk > size + align)  && space_in_chunk < smallest_space) {
      smallest_space = space_in_chunk;
      best = cur;
    }
    cur = cur->next;
  }

  return best;
}

/** given a chunk that is large enough for a `alignment` aligned allocation of size `size
 * try split in into 3.
 *
 * e.g. if the chunk is |                                                      |
 * where                |                 |XXXXXXXXXXXXX|                      |
 *                          ^^^^^^^^^         ^^^^^          ^^^^^^^^^^^^
 *                          alignment         chunk          unused space
 *
 * into                 |  chunk1         | chunk2       | chunk3              |
 * and return a pointer to chunk2
 */
valloc_free_chunk* valloc_freelist_split_alignment(valloc_free_chunk* chunk, uint64_t size, uint64_t alignment) {
  uint64_t chunk_start = (uint64_t)chunk;
  uint64_t chunk_alignment = ALIGN_UP_TO(chunk_start, alignment) - chunk_start;
  if (chunk->size < size + chunk_alignment) {
    fail("! valloc_freelist_split_alignment: cannot split a chunk that is not big enough");
  }

  /* remove the chunk and add 1-3 new ones .. */
  valloc_freelist_remove_chunk(chunk);

  uint64_t chunk1_size = chunk_alignment;
  uint64_t chunk2_size = size;
  uint64_t chunk3_size = chunk->size - chunk1_size - chunk2_size;

  valloc_freelist_allocate_free_chunk(chunk_start+chunk_alignment, size);

  if (chunk1_size > sizeof(valloc_free_chunk)) {
    valloc_freelist_allocate_free_chunk(chunk_start, chunk1_size);
  }

  if (chunk3_size > sizeof(valloc_free_chunk)) {
    valloc_freelist_allocate_free_chunk(chunk_start+chunk1_size+chunk2_size, chunk3_size);
  }

  return ((valloc_free_chunk*)(chunk_start + chunk1_size));
}

/** given a chunk, remove it from the freelist entirely
 */
void valloc_freelist_remove_chunk(valloc_free_chunk* chunk) {
  SET(chunk->prev, next, chunk->next);
  SET(chunk->next, prev, chunk->prev);

  if (chunk->prev == NULL) {
    mem.freelist = chunk->next;
  }
}

/** compact the freelist
 */
static void __compact_together(valloc_free_chunk* chunk);

/** try combine chunk1 and chunk2
 * assuming valloc_freelist_start_of_chunk(chunk1) < valloc_freelist_start_of_chunk(chunk2)
 */
static int __valloc_freelist_compact_chunk_possibility(valloc_free_chunk* chunk1, valloc_free_chunk* chunk2) {
  uint64_t chunk1_end = valloc_freelist_start_of_chunk(chunk1) + valloc_freelist_chunk_size(chunk1);
  uint64_t chunk2_start = valloc_freelist_start_of_chunk(chunk2);

  if (chunk1_end == chunk2_start) {
    /* either they touch */
    chunk1->size += chunk2->size;
    valloc_freelist_remove_chunk(chunk2);
    __compact_together(chunk1);
    return 1;
  } else if (! valloc_is_region_allocated(chunk1_end, chunk2_start)) {
    /* or the entire space between is also free */
    chunk1->size += (chunk2_start - chunk1_end) + chunk2->size;
    valloc_freelist_remove_chunk(chunk2);
    __compact_together(chunk1);
    return 1;
  }

  return 0;
}

static void __compact_together(valloc_free_chunk* chunk) {
  valloc_free_chunk* cur = mem.freelist;

  while (cur != NULL) {
    if (valloc_freelist_start_of_chunk(chunk) < valloc_freelist_start_of_chunk(cur)) {
      if (__valloc_freelist_compact_chunk_possibility(chunk, cur)) {
        break;
      }
    } else if (chunk != cur) {
      if (__valloc_freelist_compact_chunk_possibility(cur, chunk)) {
        break;
      }
    }
    cur = cur->next;
  }
}

static valloc_free_chunk* __next_free_chunk(void) {
  valloc_free_chunk* cur = mem.freelist;

  valloc_free_chunk* best = NULL;
  while (cur) {
    if ((! best) || ((uint64_t)best > (uint64_t)cur)) {
      best = cur;
    }
    cur = cur->next;
  }

  return best;
}


/** try move mem.top up until the next allocated chunk
 */
static void __shift_top(void) {
  while (1) {
    valloc_free_chunk* next = __next_free_chunk();

    if (! next)
      break;

    uint64_t chunk_start = valloc_freelist_start_of_chunk(next);
    uint64_t chunk_end = chunk_start + next->size;
    if (valloc_is_region_allocated(mem.top, chunk_start)) {
      break;
    } else {
      mem.top = chunk_end;
      valloc_freelist_remove_chunk(next);
    }
  }
}

void valloc_freelist_compact_chunk(valloc_free_chunk* chunk) {
  __compact_together(chunk);
  __shift_top();
}