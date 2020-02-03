#include <stdint.h>

#include "lib.h"

char* __free_check(uint64_t size) {
    valloc_alloc** last = &mem.freelist;
    valloc_alloc* free = mem.freelist;
    while (free) {
        if (free->size >= size) {
            *last = free->next;
            return (char*)free;
        }
        
        last = &free->next;
        free = free->next;
    }

    return 0;
}

char* __alloc_with_alignment(uint64_t size, uint64_t pow2_alignment) {
    /* first: check if there space in freelist */
    char* p = __free_check(size);
    if (p)
        return p;

    uint64_t new_top = ALIGN_POW2(mem.top - size, pow2_alignment);
    mem.top = new_top;
    mem.space_free = mem.space_free - size;
    return (char*)new_top;
}

char* alloc(uint64_t size) {
    return __alloc_with_alignment(size, 1);
}

char* alloc_aligned(uint64_t pow2_size) {
    return __alloc_with_alignment(pow2_size, pow2_size);
}

void free(char* p, uint64_t size) {
    *(valloc_alloc*)p = (valloc_alloc){
        .size = size,
        .next = mem.freelist,
    };
    mem.freelist = (valloc_alloc*)p;
}

void free_all(void) {
    mem.freelist = 0;
    mem.top = mem.end_of_mem;
    mem.space_free = TOTAL_MEM;
}

void valloc_memset(char* p, uint64_t value, uint64_t size) {
    uint64_t end = (uint64_t)p + size;
    for (; (uint64_t)p < end; p++) {
        *p = value;
    }
}