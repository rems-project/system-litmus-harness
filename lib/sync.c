#include "lib.h"

void lock(volatile int* lock) {
  asm volatile (
    "0:\n"
    "ldxr x0, [%[lock]]\n"
    "cbnz x0, 0b\n"
    "mov x0, #1\n"
    "stxr w1, x0, [%[lock]]\n"
    "cbnz w1, 0b\n"
  :
  : [lock] "r" (lock)
  : "x0", "x1", "memory"
  );

  dsb();
}

void unlock(volatile int* lock) {
  *lock = 0;
}

void bwait(int cpu, int i, uint64_t* barrier, int sz) {
  asm volatile (
    "0:\n"
    "ldxr x0, [%[bar]]\n"
    "add x0, x0, #1\n"
    "stxr w1, x0, [%[bar]]\n"
    "cbnz w1, 0b\n"
  :
  : [bar] "r" (barrier)
  : "x0", "x1", "memory"
  );

  dsb();

  if (i == cpu) {
    while (*barrier != sz) wfe();
    *barrier = 0;
    dmb();
  } else {
    while (*barrier != 0) wfe();
  }
}