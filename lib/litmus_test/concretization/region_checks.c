#include "lib.h"

uint8_t in_same_region(uint64_t va1, uint64_t va2, own_level_t size) {
  return (va1 & ~BITMASK(LEVEL_SHIFTS[size])) == (va2 & ~BITMASK(LEVEL_SHIFTS[size]));
}