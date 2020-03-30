#include "lib.h"

int get_ticket(volatile lamport_lock_t* lock) {
  unsigned int max_ticket = 0;
  for (int j = 0; j < num_of_threads; j++) {
    if (max_ticket < lock->number[j]) {
      max_ticket = lock->number[j];
    }
  }

  return 1+max_ticket;
}

int lex_less(volatile lamport_lock_t* lock, int tid1, int tid2) {
  return ((lock->number[tid1] < lock->number[tid2]) || ((lock->number[tid1] == lock->number[tid2])
                                                        && tid1 < tid2));
}

void lamport_lock(volatile lamport_lock_t* lock) {
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
}

void lamport_unlock(volatile lamport_lock_t* lock) {
  int i = get_cpu();
  dmb();
  lock->number[i] = 0;
  dmb();
}

void mutex_lock(volatile mutex_t* m) {
  asm volatile(
    "0:\n"
    "ldxr w0, [%[lock]]\n"
    "cbnz w0, 0b\n"
    "mov w0, #1\n"
    "stxr w1, w0, [%[lock]]\n"
    "cbnz w1, 0b\n"
    :
    : [lock] "r" (&m->m)
    : "x0", "x1", "memory");

  /* ensure ctrl-deps on the lock are observed */
  dsb();
}

void mutex_unlock(volatile mutex_t* m) {
  m->m = 0;
  dmb();
}

void lock(volatile lock_t* lock) {
  if (ENABLE_PGTABLE)
    mutex_lock(&lock->m);
  else
    lamport_lock(&lock->l);
}

void unlock(volatile lock_t* lock) {
  if (ENABLE_PGTABLE)
    mutex_unlock(&lock->m);
  else
    lamport_unlock(&lock->l);
}

lock_t bwait_lock;

void bwait(int cpu, int i, bar_t* bar, int sz) {
  /* slow acquire */
  lock(&bwait_lock);
  bar->counter++;
  if (bar->counter == sz) {
    bar->released = 1;
    dsb();
    sev();
  }
  unlock(&bwait_lock);

  while (! bar->released) wfe();
  bar->release_flags[get_cpu()] = 1;
  dsb();
  
  for (int i = 0; i < sz; i++) {
    while (! bar->release_flags[i]) dmb();
  }
}

void mmu_lock(volatile lock_t* lock) {
  asm volatile(
    "0:\n"
    "ldxr w0, [%[lock]]\n"
    "cbnz w0, 0b\n"
    "mov w0, #1\n"
    "stxr w1, w0, [%[lock]]\n\t"
    "dsb nsh\n\t"
    "cbnz w1, 0b\n"
    :
    : [lock] "r"(lock)
    : "x0", "x1", "memory");
}