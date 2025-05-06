#ifndef SYNC_H
#define SYNC_H

#include "device.h"

/** before the MMU is turned on
 * cannot use exclusives for atomic lock
 * so use lamport bakery algorithm instead
 */
typedef struct
{
  int entering[MAX_CPUS];
  unsigned int number[MAX_CPUS];
} lamport_lock_t;

/* mutex */
void lamport_lock(volatile lamport_lock_t* lock);
void lamport_unlock(volatile lamport_lock_t* lock);

/** once MMU is on
 * can't use lamport lock anymore as it's not thread-safe
 * so switch to exclusives
 */
typedef struct
{
  u64 locked;
} mutex_t;

void mutex_lock(volatile mutex_t* lock);
void mutex_unlock(volatile mutex_t* lock);

/** if using ENABLE_PGTABLE don't use lock_t until after the MMU is switched on.
 */
typedef struct
{
  union {
    mutex_t m;
    lamport_lock_t l;
  };
} lock_t;

void lock(volatile lock_t* lock);
void unlock(volatile lock_t* lock);

#define LOCK(lockptr)                                                   \
  ({                                                                    \
    DEBUG(DEBUG_LOCKS, "LOCK(%s=%p)\n", STR_LITERAL(lockptr), lockptr); \
    lock(lockptr);                                                      \
  })

#define UNLOCK(lockptr)                                                   \
  ({                                                                      \
    DEBUG(DEBUG_LOCKS, "UNLOCK(%s=%p)\n", STR_LITERAL(lockptr), lockptr); \
    unlock(lockptr);                                                      \
  })

/* barrier */
typedef struct
{
  volatile u64 iteration;
  volatile u64 waiting;
  u8 current_state;
} bar_t;

#define EMPTY_BAR \
  (bar_t) { 0, 0, 0 }

void bwait(int cpu, bar_t* barrier, int sz);

#define BWAIT(cpu, barrier, sz)                                                                                    \
  ({                                                                                                               \
    DEBUG(DEBUG_BWAITS, "BWAIT(%s=%d, %s=%p, sz=%d)\n", STR_LITERAL(cpu), cpu, STR_LITERAL(barrier), barrier, sz); \
    bwait(cpu, barrier, sz);                                                                                       \
  })

#endif /* SYNC_H */
