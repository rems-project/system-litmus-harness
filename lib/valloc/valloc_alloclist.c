#include <stdint.h>

#include "lib.h"

/** move a chunk from the dealloc list to the alloc list */
static void swap(valloc_alloc_chunk* chunk, valloc_alloc_chunk** from, valloc_alloc_chunk** to) {
  if (chunk == NULL) {
    fail("valloc_swap : null chunk");
    return;
  }

  if (chunk->prev) {
    chunk->prev->next = chunk->next;
    SET(chunk->next, prev, chunk->prev);
  } else {
    *from = chunk->next;
    UNSET(chunk->next, prev);
  }

  if (*to) {
    chunk->next = *to;
    chunk->prev = NULL;
    (*to)->prev = chunk;
    *to = chunk;
  } else {
    chunk->next = NULL;
    chunk->prev = NULL;
    *to = chunk;
  }
}

static void move_to_alloc(valloc_mempool* pool, valloc_alloc_chunk* chunk) {
  swap(chunk, &pool->chunk_unalloc_list, &pool->chunk_alloc_list);
}

static void move_to_dealloc(valloc_mempool* pool, valloc_alloc_chunk* chunk) {
  swap(chunk, &pool->chunk_alloc_list, &pool->chunk_unalloc_list);
}

valloc_alloc_chunk* valloc_alloclist_find_alloc_chunk(valloc_mempool* pool, uint64_t addr) {
  valloc_alloc_chunk* cur = pool->chunk_alloc_list;
  while (cur) {
    uint64_t start = cur->start;
    uint64_t end = start + cur->size;
    if (start <= addr && addr < end)
      return cur;

    cur = cur->next;
  }

  return NULL;
}

void valloc_alloclist_dealloc(valloc_mempool* pool, uint64_t addr) {
  valloc_alloc_chunk* alloced_chunk = valloc_alloclist_find_alloc_chunk(pool, addr);
  if (! alloced_chunk) {
    fail("! err: valloc_alloclist_dealloc no alloc at %p (double free?)\n", addr);
  }
  move_to_dealloc(pool, alloced_chunk);
}

valloc_alloc_chunk* valloc_alloclist_alloc(valloc_mempool* pool, uint64_t addr, uint64_t size) {
  /* assume addr not already allocated */
  valloc_alloc_chunk* head = pool->chunk_unalloc_list;
  if (head == NULL) {
    fail("! err: cannot alloc any more chunks\n");
  }

  head->start = addr;
  head->size = size;
  move_to_alloc(pool, head);

  return head;
}

/** returns 1 if any of the region [start, end] (inclusive)
 * is allocated
 */
uint8_t valloc_is_region_allocated(valloc_mempool* pool, uint64_t start, uint64_t end) {
  valloc_alloc_chunk* cur = pool->chunk_alloc_list;

  while (cur) {
    uint64_t blk_start = cur->start;
    if (start <= blk_start && blk_start <= end) {
      return 1;
    }
    cur = cur->next;
  }

  return 0;
}