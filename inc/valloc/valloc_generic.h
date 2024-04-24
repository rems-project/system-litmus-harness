#ifndef VALLOC_H
#define VALLOC_H

/* generic allocator
 *
 * used for allocating non-pagetable data during execution of the harness
 */

#define UNSET(a,b) do{if (a) {a->b=NULL;}} while (0);
#define SET(a,b,c) do{if (a) {a->b=(c);}} while (0);

typedef struct __free_chunk {
    u64 size;
    struct __free_chunk* prev;
    struct __free_chunk* next;
} valloc_free_chunk;

#if DEBUG_ALLOC_META
typedef struct {
    u64 ts;    /* timestamp of allocation */
    u64 where; /* address of ALLOC() call */
} valloc_alloc_chunk_debug_metadata;
#endif

typedef struct __alloc {
    u64 start;
    u64 size;
    struct __alloc* prev;
    struct __alloc* next;
#if DEBUG_ALLOC_META
    valloc_alloc_chunk_debug_metadata meta;
#endif
} valloc_alloc_chunk;


/** we store a list of all allocations so far
 * with a fixed maximum number of allocations
 */
#define NUM_ALLOC_CHUNKS 1024

/** the memory itself is just a moving bar that
 * goes from the top of memory down towards the top
 * of stack
 */
typedef struct {
    u64 top;
    valloc_free_chunk* freelist;
    valloc_alloc_chunk chunks[NUM_ALLOC_CHUNKS];
    valloc_alloc_chunk* chunk_unalloc_list;
    valloc_alloc_chunk* chunk_alloc_list;
} valloc_mempool;

/* global memory pool to allocate from */
extern valloc_mempool mem;

valloc_alloc_chunk* valloc_alloclist_find_alloc_chunk(valloc_mempool* pool, u64 addr);
void valloc_alloclist_dealloc(valloc_mempool* pool, u64 addr);
valloc_alloc_chunk* valloc_alloclist_alloc(valloc_mempool* pool, u64 addr, u64 size);
u8 valloc_is_region_allocated(valloc_mempool* pool, u64 start, u64 end);

/** free'd memory is stored as a linked list in a dedicated free list
 */
valloc_free_chunk* valloc_freelist_find_best(u64 size, u64 alignment);
valloc_free_chunk* valloc_freelist_split_alignment(valloc_free_chunk* chunk, u64 size, u64 alignment);
void valloc_freelist_allocate_free_chunk(u64 addr, u64 size);
void valloc_freelist_remove_chunk(valloc_free_chunk* chunk);
void valloc_freelist_compact_chunk(valloc_free_chunk* chunk);

#define ALLOC_SIZE(p) \
  (valloc_alloclist_find_alloc_chunk(&mem, (u64)(p))->size)

void init_valloc(void);

#define __ALLOC_MANY(var, ty, count) ({ \
  void* var = alloc_with_alignment((sizeof(ty)*(count)), sizeof(ty)); \
  DEBUG(DEBUG_ALLOCS, "alloc %ldx %s @ %p\n", (count), #ty, var); \
  var; \
})

#define ALLOC_MANY(ty, count) \
  __ALLOC_MANY(FRESH_VAR, ty, count)

#define ALLOC_ONE(ty) ALLOC_MANY(ty, 1)

#define __ALLOC_SIZED(var, size) ({ \
  void* var = alloc((size)); \
  DEBUG(DEBUG_ALLOCS, "alloc %ld B @ %p\n", (size), var); \
  var; \
})

#define ALLOC_SIZED(size) \
  __ALLOC_SIZED(FRESH_VAR, size)

#define FREE(p) ({ \
  DEBUG(DEBUG_ALLOCS, "free %p\n", (p)); \
  free((p)); \
})

void* alloc_with_alignment(u64 size, u64 alignment);
void* alloc(u64 size);
void* realloc(void* p, u64 new_size);

void free(void* p);
void valloc_memset(void* p, u8 value, u64 size);
void valloc_memcpy(void* dest, void* src, u64 size);

u64 valloc_free_size(void);

/** returns 1 if the addr p is in a free chunk
 * (0 means either it's allocated or in unreachable zones of memory)
 */
int valloc_is_free(void* p);

/* useful functions for touching a valloc_free_chunk */
u64 valloc_freelist_start_of_chunk(valloc_free_chunk*);
u64 valloc_freelist_chunk_size(valloc_free_chunk* chunk);

/* debugging functions */

/** count the number of currently allocated chunks
 */
u64 valloc_alloclist_count_chunks(void);

#endif /* VALLOC_H */
