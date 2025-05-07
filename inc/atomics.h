#ifndef ATOMICS_H
#define ATOMICS_H

#include "lib.h"

/**
 * force a single write with release semantics
 */
static inline void write_release(volatile u64* loc, u64 val) {
  /* clang-format off */
  asm volatile (
    "stlr %[val],[%[loc]]\n"
  :
  : [loc] "r" (loc), [val] "r" (val)
  : "memory", "cc"
  );
  /* clang-format on */
}

/**
 * force a single single-copy-atomic write
 */
static inline void write_once(volatile u64* loc, u64 val) {
  /* clang-format off */
  asm volatile (
    "str %[val],[%[loc]]\n"
  :
  : [loc] "r" (loc), [val] "r" (val)
  : "memory", "cc"
  );
  /* clang-format on */
}

#endif /* ATOMICS_H */