#include "lib.h"

int log2(u64 n) {
  int k = 0;

  if (n == 0)
    return 0;

  while (1) {
    if (n == 1) {
      return k;
    }

    n >>= 1;
    k++;
  }
}

u32 read_be(char* p) {
  u8* pc = (u8*)p;
  u8 b0 = *pc;
  u8 b1 = *(pc + 1);
  u8 b2 = *(pc + 2);
  u8 b3 = *(pc + 3);
  return ((u32)b0 << 24) + ((u32)b1 << 16) + ((u32)b2 << 8) + (u32)b3;
}

u64 read_be64(char* p) {
  u8* pc = (u8*)p;

  u8 bs[] = { *(pc + 7), *(pc + 6), *(pc + 5), *(pc + 4), *(pc + 3), *(pc + 2), *(pc + 1), *(pc + 0) };

  u64 x = 0;
  for (int i = 7; i >= 0; i--) {
    x <<= 8;
    x += bs[i];
  }
  return x;
}