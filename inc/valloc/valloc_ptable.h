#ifndef VALLOC_PTABLE_H
#define VALLOC_PTABLE_H

/* allocator for contiguous page-sized chunks
 *
 * used for allocating pagetables during a test
 */

typedef struct
{
  u64 bot;
} valloc_ptable_mem;

/** first-time initialise the pagetable allocator
 */
void init_valloc_ptable(void);

/** save a checkpoint for the current allocation
 * which can later be used to reset the allocator
 * back to this state
 */
valloc_ptable_mem valloc_ptable_checkpoint(void);

/** restore the allocator back to an earlier checkpointed state
 */
void valloc_ptable_restore(valloc_ptable_mem mem);

/** allocate one zero'd page of memory
 */
void* zalloc_ptable(void);

#endif /* VALLOC_PTABLE_H */
