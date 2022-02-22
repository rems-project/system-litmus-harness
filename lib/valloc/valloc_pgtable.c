#include "lib.h"

static valloc_ptable_mem ptable_mem;

/** this lock protects the ptable_mem
 */
static lock_t __valloc_pgtable_lock;

/** save a checkpoint for the current allocation
 * which can later be used to reset the allocator
 * back to this state
 */
valloc_ptable_mem valloc_ptable_checkpoint(void) {
  return ptable_mem;
}

void init_valloc_ptable(void) {
  LOCK(&__valloc_pgtable_lock);
  ptable_mem.bot = BOT_OF_PTABLES;
  UNLOCK(&__valloc_pgtable_lock);
}

/** restore the allocator back to an earlier checkpointed state
 */
void valloc_ptable_restore(valloc_ptable_mem checkpoint) {
  LOCK(&__valloc_pgtable_lock);
  ptable_mem = checkpoint;
  UNLOCK(&__valloc_pgtable_lock);
}

/** allocate one zero'd page of memory
 */
void* zalloc_ptable(void) {
  LOCK(&__valloc_pgtable_lock);
  u64 cur = ptable_mem.bot;
  ptable_mem.bot += PAGE_SIZE;
  UNLOCK(&__valloc_pgtable_lock);

  if (cur >= TOP_OF_PTABLES) {
    fail("! cannot allocate any more pagetables\n");
  }

  valloc_memset((void*)cur, 0, PAGE_SIZE);
  return (void*)cur;
}