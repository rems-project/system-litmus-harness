#include "lib.h"
#include "testlib.h"

int valloc_freelist_hash(valloc_free_chunk* freelist) {
  uint64_t hash = 0;

  while (freelist) {
    hash ^= (uint64_t)freelist + ((freelist->size) << 12);
    hash = (hash >> 1) | ((hash & 1) << 64);
    freelist = freelist->next;
  }

  return hash;
}