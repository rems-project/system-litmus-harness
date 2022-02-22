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