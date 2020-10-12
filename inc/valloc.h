#ifndef VALLOC_H
#define VALLOC_H
#include <stdint.h>

#define UNSET(a,b) do{if (a) {a->b=NULL;}} while (0);
#define SET(a,b,c) do{if (a) {a->b=(c);}} while (0);

typedef struct __free_chunk {
    uint64_t size;
    struct __free_chunk* prev;
    struct __free_chunk* next;
} valloc_free_chunk;

typedef struct {
    uint64_t ts; /* timestamp of allocation */
} valloc_alloc_chunk_debug_metadata;

typedef struct __alloc {
    uint64_t start;
    uint64_t size;
    struct __alloc* prev;
    struct __alloc* next;
    valloc_alloc_chunk_debug_metadata meta;
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
    uint64_t top;
    valloc_free_chunk* freelist;
    valloc_alloc_chunk chunks[NUM_ALLOC_CHUNKS];
    valloc_alloc_chunk* chunk_unalloc_list;
    valloc_alloc_chunk* chunk_alloc_list;
} valloc_mempool;

/* global memory pool to allocate from */
extern valloc_mempool mem;

valloc_alloc_chunk* valloc_alloclist_find_alloc_chunk(valloc_mempool* pool, uint64_t addr);
void valloc_alloclist_dealloc(valloc_mempool* pool, uint64_t addr);
valloc_alloc_chunk* valloc_alloclist_alloc(valloc_mempool* pool, uint64_t addr, uint64_t size);
uint8_t valloc_is_region_allocated(valloc_mempool* pool, uint64_t start, uint64_t end);

/** free'd memory is stored as a linked list in a dedicated free list
 */
valloc_free_chunk* valloc_freelist_find_best(uint64_t size, uint64_t alignment);
valloc_free_chunk* valloc_freelist_split_alignment(valloc_free_chunk* chunk, uint64_t size, uint64_t alignment);
void valloc_freelist_allocate_free_chunk(uint64_t addr, uint64_t size);
void valloc_freelist_remove_chunk(valloc_free_chunk* chunk);
void valloc_freelist_compact_chunk(valloc_free_chunk* chunk);

void init_valloc(void);

#define ALLOC_MANY(ty, count) ({ \
  void* v = alloc_with_alignment((sizeof(ty))*count, sizeof(ty)); \
  if (DEBUG_ALLOCS) { \
    debug("alloc %ldx %s @ %p\n", count, #ty, v); \
  } \
  v; \
})

#define ALLOC_ONE(ty) ALLOC_MANY(ty, 1)

void* alloc_with_alignment(uint64_t size, uint64_t alignment);
void* alloc(uint64_t size);

void free(void* p);
void valloc_memset(void* p, uint64_t value, uint64_t size);
void valloc_memcpy(void* dest, void* src, uint64_t size);

uint64_t valloc_free_size(void);

/** returns 1 if the addr p is in a free chunk
 * (0 means either it's allocated or in unreachable zones of memory)
 */
int valloc_is_free(void* p);

/* useful functions for touching a valloc_free_chunk */
uint64_t valloc_freelist_start_of_chunk(valloc_free_chunk*);
uint64_t valloc_freelist_chunk_size(valloc_free_chunk* chunk);

#endif /* VALLOC_H */
