#ifndef VALLOC_ARENA_H
#define VALLOC_ARENA_H

#include "ints.h"

typedef struct {
  u64 top;
  u64 size;
  char data[];
} valloc_arena;

void arena_init(valloc_arena *arena, u64 size);
void *__alloc_arena(valloc_arena *arena, u64 size);

#define __ALLOC_ARENA(VAR, ARENA, SIZE) ({\
  void* VAR = __alloc_arena((ARENA), (SIZE)); \
  DEBUG(DEBUG_ALLOCS, "alloc in arena %p, %ld B @ %p\n", (ARENA), (SIZE), VAR); \
  VAR; \
})

#define ALLOC_ARENA(ARENA, SIZE) \
  __ALLOC_ARENA(FRESH_VAR, ARENA, SIZE)

#define __ALLOC_ARENA_MANY(var, arena, ty, count) ({ \
  void* var = __alloc_arena((arena), (sizeof(ty)*(count))); \
  DEBUG(DEBUG_ALLOCS, "alloc in arena %p, %ldx %s @ %p\n", (arena), (count), #ty, var); \
  var; \
})

#define ALLOC_ARENA_MANY(arena, ty, count) \
  __ALLOC_ARENA_MANY(FRESH_VAR, arena, ty, count)

#endif /* VALLOC_ARENA_H */
