#include "lib.h"

/**
 * ARM64 locks
 *
 *  if the MMU is switched off then use a lamport lock
 *  otherwise,  have to use the atomic mutex
 */

void lock(volatile lock_t* lock) {
  if (!current_thread_info()->locking_enabled)
    return;

  if (ENABLE_PGTABLE)
    mutex_lock(&lock->m);
  else
    lamport_lock(&lock->l);
}

void unlock(volatile lock_t* lock) {
  if (!current_thread_info()->locking_enabled)
    return;

  if (ENABLE_PGTABLE)
    mutex_unlock(&lock->m);
  else
    lamport_unlock(&lock->l);
}

/** lamport lock
*/
int get_ticket(volatile lamport_lock_t* lock) {
  unsigned int max_ticket = 0;
  for (int j = 0; j < NO_CPUS; j++) {
    if (max_ticket < lock->number[j]) {
      max_ticket = lock->number[j];
    }
  }

  return 1 + max_ticket;
}

int lex_less(volatile lamport_lock_t* lock, int tid1, int tid2) {
  return ((lock->number[tid1] < lock->number[tid2]) || ((lock->number[tid1] == lock->number[tid2]) && tid1 < tid2));
}

void lamport_lock(volatile lamport_lock_t* lock) {
  int i = get_cpu();
  lock->entering[i] = 1;
  dmb();
  lock->number[i] = get_ticket(lock);
  dmb();
  lock->entering[i] = 0;
  for (int j = 0; j < NO_CPUS; j++) {
    while (lock->entering[j]) {
      /* nothing */
    }
    dmb();
    while ((lock->number[j] != 0) && lex_less(lock, j, i)) {
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

/** arm64 atomic lock
 */

void __atomic_cas(volatile u64* va, u64 old, u64 new) {
  /* atomic test and update
   * equivalent to an atomic:
   * <while (*va != old); *va = new>;
   */
  asm volatile(
    /* dont wait on first read */
    "sevl\n"

    /* skip to first read */
    "b 1f\n"
    "0:\n"
    /* if the load-exclusive failed to read old,
     * then clear the exclusives manually, so as not to
     * block any other read/write
     */
    "clrex\n"
    "1:\n"
    "wfe\n"
    "ldxr x0, [%[va]]\n"
    "cmp x0, %[old]\n"
    "b.ne 0b\n"
    "dsb sy\n"
    "stxr w1, %[val], [%[va]]\n"
    "cbnz w1, 1b\n"

    "1:\n"
    "sev\n"
    :
    : [va] "r"(va), [val] "r"(new), [old] "r"(old)
    : "memory", "x0", "x1"
  );
}

void mutex_lock(volatile mutex_t* mut) {
  /* acquire mutex */
  __atomic_cas(&mut->locked, 0, 1);
}

void mutex_unlock(volatile mutex_t* mut) {
  /* release the mutex */
  /* NB: A53 errata 855872
   * since the .data section is mapped inner/outer writeback and cacheable
   * we must use an atomic store exclusive here rather than a plain store
   *
   * for simplicitly we use the helper __atomic_cas
   * which also ensures UNLOCK() requires a LOCK()d mutex first.
   */
  __atomic_cas(&mut->locked, 1, 0);
}

/**
 * barriers
 *
 * bwait() is used to ensure multiple threads synchronize at the same point
 * releasing them all at roughly the same time.
 */

lock_t bwait_lock;

void __atomic_dec(volatile u64* va) {
  /* atomic decrement
   */
  asm volatile(
    /* dont wait on initial attempt */
    "sevl\n"

    /* try decrement */
    "0:\n"
    "wfe\n"
    "ldxr x0, [%[va]]\n"
    "sub x0, x0, #1\n"
    "dsb sy\n\t"
    "stxr w1, x0, [%[va]]\n"
    /*
     * try again if another thread intervened
     */
    "cbnz w1, 0b\n"
    "1:"
    /* decremented */
    "sev\n"
    :
    : [va] "r"(va)
    : "memory", "x0", "x1"
  );
}

void bwait(int vcpu, bar_t* bar, int sz) {
  if (!current_thread_info()->locking_enabled) {
    fail("bwait needs locking enabled\n");
  }

  /* slow acquire */
  LOCK(&bwait_lock);

  /* if waiting is non-zero then
   * then we're still waiting for the release of one of the previous
   * bwait()s
   */
  if (bar->current_state == 1) {
    while (bar->waiting != 0) {
      /* because another thread may be waiting on a *different* BWAIT
       * at the same time, we have to ensure it gets a chance to finish and reach this BWAIT too
       */
      UNLOCK(&bwait_lock);
      dmb();
      LOCK(&bwait_lock);
    }

    bar->iteration++;
    bar->current_state = 0;
  }

  bar->waiting++;

  if (bar->waiting == sz) {
    /* release them all */
    bar->current_state = 1;
    dmb();
    sev();
  } else {
    sevl();
  }

  UNLOCK(&bwait_lock);

  /* wait for the last one to arrive and flip the current state */
  u64 iter = bar->iteration;
  while (bar->current_state == 0)
    wfe();
  __atomic_dec(&bar->waiting);

  /** a slow bwait release section
   * this is to try reduce overhead and get all threads spinning at once
   * before releasing
   * N.B.  context switching kills this, it really needs a dedicated CPU.
   *
   * vcpus are labelled 0..N for N <= sz
   */
  while (bar->iteration == iter && bar->waiting != 0)
    dmb();
}