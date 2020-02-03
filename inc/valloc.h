#include <stdint.h>

typedef struct __alloc {
    uint64_t size;
    struct __alloc* next;
} valloc_alloc;

typedef struct {
    uint64_t end_of_mem;

    uint64_t top;

    /* artificially bound to catch errors */
    uint64_t space_free;

    valloc_alloc* freelist;
} valloc_mempool;

#define TOP_OF_MEM 0x40080000
#define TOTAL_MEM 4096*20

/* global memory pool to allocate from */
static valloc_mempool mem = {
    .end_of_mem = TOP_OF_MEM,
    .top = TOP_OF_MEM,
    .space_free = TOTAL_MEM,
    .freelist = 0,
};

char* alloc(uint64_t size);
char* alloc_aligned(uint64_t pow2_size);
void free(char* p, uint64_t size);
void free_all(void);

void valloc_memset(char* p, uint64_t value, uint64_t size);