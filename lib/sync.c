#include "lib.h"

int get_ticket(volatile lock_t* lock) {
  unsigned int max_ticket = 0;
  for (int j = 0; j < num_of_threads; j++) {
    if (max_ticket < lock->number[j]) {
      max_ticket = lock->number[j];
    }
  }

  return 1+max_ticket;
}

int lex_less(volatile lock_t* lock, int tid1, int tid2) {
  return ((lock->number[tid1] < lock->number[tid2]) || ((lock->number[tid1] == lock->number[tid2])
                                                        && tid1 < tid2));
}

void lock(volatile lock_t* lock) {
  int i = get_cpu();
  lock->entering[i] = 1;
  lock->number[i] = get_ticket(lock);
  lock->entering[i] = 0;
  for (int j = 0; j < num_of_threads; j++) {
    while (lock->entering[j]) {
      /* nothing */
    }
    while ((lock->number[j] != 0) &&
           lex_less(lock, j, i)) {
      /* nothing */
    }
  }
  
  /*asm volatile(
      "0:\n"
      "ldxr w0, [%[lock]]\n"
      "cbnz w0, 0b\n"
      "mov w0, #1\n"
      "stxr w1, w0, [%[lock]]\n"
      "cbnz w1, 0b\n"
      :
      : [lock] "r"(lock)
      : "x0", "x1", "memory");
  */
  dsb();
}

void unlock(volatile lock_t* lock) {
  int i = get_cpu();
  lock->number[i] = 0;  
}

lock_t bwait_lock;

void bwait(int cpu, int i, volatile int* barrier, int sz) {
  lock(&bwait_lock);
  *barrier += 1;
  unlock(&bwait_lock);
  /*
  asm volatile(
      "0:\n"
      "ldxr w0, [%[bar]]\n"
      "add w0, w0, #1\n"
      "stxr w1, w0, [%[bar]]\n"
      "cbnz w1, 0b\n"
      :
      : [bar] "r"(barrier)
      : "x0", "x1", "memory");
      
  dsb();
  */
  
  if (i == cpu) {
    while (*barrier != sz) wfe();
    *barrier = 0;
    dmb();
    sev();
  } else {
    while (*barrier != 0) {
      wfe();
    }
  }
}
