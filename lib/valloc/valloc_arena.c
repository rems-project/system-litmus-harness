
#include "lib.h"

/* one big lock for all arena allocations
 */
lock_t __valloc_arena_lock;

void arena_init(valloc_arena* arena, u64 size) {
  LOCK(&__valloc_arena_lock);
  arena->top = (u64)(&arena->data[0]);
  arena->size = size;
  UNLOCK(&__valloc_arena_lock);
}

void* __alloc_arena(valloc_arena* arena, u64 size) {
  LOCK(&__valloc_arena_lock);
  u64 cur = arena->top;

  /* move the top, arenas grow up unlike the heap
   * make sure to be aligned, but don't over-align */
  cur = ALIGN_UP(cur, MIN(8, log2(size)));
  arena->top = cur + size;

  u64 allocated = arena->top - (u64)&arena->data[0];
  if (allocated > arena->size)
    fail("! over-allocated arena\n");

  UNLOCK(&__valloc_arena_lock);
  return (void*)cur;
}