#include "lib.h"

/**
 * ARM64 locks
 *
 *  if the MMU is switched off then use a lamport lock
 *  otherwise,  have to use the atomic mutex
 */

void lock(volatile lock_t* lock) {
  if (! current_thread_info()->locking_enabled)
    return;

  if (ENABLE_PGTABLE)
    mutex_lock(&lock->m);
  else
    lamport_lock(&lock->l);
}

void unlock(volatile lock_t* lock) {
  if (! current_thread_info()->locking_enabled)
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

/** arm64 atomic lock
 */

void __atomic_wait_and_set(volatile uint64_t* va, uint64_t v) {
  /* atomic test and update
   * equivalent to an atomic:
   * <while (!*va); *va = v>;
   */
  asm volatile(
    "0:\n"
    "ldxr w0, [%[va]]\n"
    "cbnz w0, 0b\n"
    "stxr w1, %[val], [%[va]]\n"
    "cbnz w1, 0b\n"
    :
    : [va] "r" (va), [val] "r" (v)
    : "memory", "x0", "x1"
  );
}

void mutex_lock(volatile mutex_t* mut) {
  /* acquire mutex */
  __atomic_wait_and_set(&mut->locked, 1);

  /* ensure critical section waits */
  dmb();
}

void mutex_unlock(volatile mutex_t* mut) {
  /* wait for critical section to finish */
  dmb();

  /* release the mutex */
  mut->locked = 0;
}

/**
 * barriers
 *
 * bwait() is used to ensure multiple threads synchronize at the same point
 * releasing them all at roughly the same time.
 */

lock_t bwait_lock;

void __atomic_dec(volatile uint64_t* va) {
  /* atomic decrement
   */
  asm volatile(
    "0:\n"
    "ldxr x0, [%[va]]\n"
    "sub x0, x0, #1\n"
    "stxr w1, x0, [%[va]]\n"
    "cbnz w1, 0b\n"
    :
    : [va] "r" (va)
    : "memory", "x0", "x1"
  );
}

void bwait(int vcpu, bar_t* bar, int sz) {
  if (! current_thread_info()->locking_enabled) {
    fail("bwait needs locking enabled\n");
  }

  /* slow acquire */
  lock(&bwait_lock);

  /* if waiting is non-zero then
   * then we're still waiting for the release of one of the previous
   * bwait()s
   */
  if (bar->current_state == 1) {
    while (bar->waiting != 0) dmb();

    bar->iteration++;
    bar->current_state = 0;
  }

  bar->waiting++;
  if (bar->waiting == sz) {
    bar->current_state = 1;
    dmb();
    sev();
  }
  unlock(&bwait_lock);

  /* wait for the last one to arrive and flip the current state */
  uint64_t iter = bar->iteration;
  while (bar->current_state == 0) wfe();
  __atomic_dec(&bar->waiting);

  /** a slow bwait release section
   * this is to try reduce overhead and get all threads spinning at once
   * before releasing
   * N.B.  context switching kills this, it really needs a dedicated CPU.
   *
   * vcpus are labelled 0..N for N <= sz
   */
  while (bar->iteration == iter && bar->waiting != 0) dmb();
}