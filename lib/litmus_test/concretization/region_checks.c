#include "lib.h"

u8 in_same_region(u64 va1, u64 va2, own_level_t size) {
  return (va1 & ~BITMASK(LEVEL_SHIFTS[size])) == (va2 & ~BITMASK(LEVEL_SHIFTS[size]));
}