
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

valloc_alloc_chunk* valloc_alloclist_find_alloc_chunk(valloc_mempool* pool, u64 addr) {
  valloc_alloc_chunk* cur = pool->chunk_alloc_list;
  while (cur) {
    u64 start = cur->start;
    u64 end = start + cur->size;
    if (start <= addr && addr < end)
      return cur;

    cur = cur->next;
  }

  return NULL;
}

void valloc_alloclist_dealloc(valloc_mempool* pool, u64 addr) {
  valloc_alloc_chunk* alloced_chunk = valloc_alloclist_find_alloc_chunk(pool, addr);
  if (!alloced_chunk) {
    fail("! err: valloc_alloclist_dealloc no alloc at %p (double free?)\n", addr);
  }
  move_to_dealloc(pool, alloced_chunk);
}

#if DEBUG_ALLOC_META
char __valloc_alloclist_stack_buf[1024];
#endif

valloc_alloc_chunk* valloc_alloclist_alloc(valloc_mempool* pool, u64 addr, u64 size) {
  /* assume addr not already allocated */
  valloc_alloc_chunk* head = pool->chunk_unalloc_list;
  if (head == NULL) {
    if (DEBUG && DEBUG_ALLOC_META)
      debug_show_valloc_mem();
    fail("! err: cannot alloc any more chunks\n");
  }

  head->start = addr;
  head->size = size;

#if DEBUG_ALLOC_META
  stack_t* stack_buf = (stack_t*)__valloc_alloclist_stack_buf;
  clear_stack(stack_buf);
  collect_stack(stack_buf);
  head->meta.where = stack_buf->frames[3].ret;
  head->meta.ts = read_clk();
#endif

  move_to_alloc(pool, head);
#if DEBUG_ALLOC_META
  DEBUG(
    DEBUG_ALLOC_META, "alloc new chunk for %p with size = %ld (chunk @ %p) from %p\n", addr, size, head, head->meta.where
  );
#endif
  return head;
}

/** returns 1 if any of the region [start, end] (inclusive)
 * is allocated
 */
u8 valloc_is_region_allocated(valloc_mempool* pool, u64 start, u64 end) {
  valloc_alloc_chunk* cur = pool->chunk_alloc_list;

  while (cur) {
    u64 blk_start = cur->start;
    if (start <= blk_start && blk_start <= end) {
      return 1;
    }
    cur = cur->next;
  }

  return 0;
}