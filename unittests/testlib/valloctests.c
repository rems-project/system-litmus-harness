#include "lib.h"
#include "testlib.h"

int valloc_freelist_hash(valloc_free_chunk* freelist) {
  u64 hash = 0;

  while (freelist) {
    hash ^= (u64)freelist + ((freelist->size) << 12);
    hash = (hash >> 1) | ((hash & 1) << 63);
    freelist = freelist->next;
  }

  return hash;
}