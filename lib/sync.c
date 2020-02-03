#include "lib.h"

void lock(volatile int* lock) {
  asm volatile (
    "0:\n"
    "ldxr w0, [%[lock]]\n"
    "cbnz w0, 0b\n"
    "mov w0, #1\n"
    "stxr w1, w0, [%[lock]]\n"
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

void bwait(int cpu, int i, volatile int* barrier, int sz) {
  asm volatile (
    "0:\n"
    "ldxr w0, [%[bar]]\n"
    "add w0, w0, #1\n"
    "stxr w1, w0, [%[bar]]\n"
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