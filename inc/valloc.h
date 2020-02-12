#include <stdint.h>

typedef struct __alloc {
    uint64_t size;
    struct __alloc* next;
} valloc_alloc;

typedef struct {
    uint64_t top;

    /* artificially bound to catch errors */
    uint64_t space_free;

    valloc_alloc* freelist;
} valloc_mempool;

/* global memory pool to allocate from */
valloc_mempool mem;

void init_valloc(void);

void* alloc_with_alignment(uint64_t size, uint64_t alignment);
void* alloc(uint64_t size);
void* alloc_aligned(uint64_t pow2_size);
void free(void* p, uint64_t size);
void free_all(void);

void valloc_memset(void* p, uint64_t value, uint64_t size);