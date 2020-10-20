#ifndef SYNC_H
#define SYNC_H
#define num_of_threads 4

/** before the MMU is turned on
 * cannot use exclusives for atomic lock
 * so use lamport bakery algorithm instead
 */
typedef struct {
  int entering[num_of_threads];
  unsigned int number[num_of_threads];
} lamport_lock_t;

/* mutex */
void lamport_lock(volatile lamport_lock_t* lock);
void lamport_unlock(volatile lamport_lock_t* lock);

/** once MMU is on
 * can't use lamport lock anymore as it's not thread-safe
 * so switch to exclusives
 */
typedef struct {
  uint64_t locked;
} mutex_t;

void mutex_lock(volatile mutex_t* lock);
void mutex_unlock(volatile mutex_t* lock);

/** if using ENABLE_PGTABLE don't use lock_t until after the MMU is switched on.
 */
typedef struct {
  union {
    mutex_t m;
    lamport_lock_t l;
  };
} lock_t;

void lock(volatile lock_t* lock);
void unlock(volatile lock_t* lock);

#define LOCK(lockptr) ({ \
  if (DEBUG_LOCKS) { \
    debug("LOCK(%s=%p)\n", STR_LITERAL(lockptr), lockptr); \
  } \
  lock(lockptr); \
})

#define UNLOCK(lockptr) ({ \
  if (DEBUG_LOCKS) { \
    debug("UNLOCK(%s=%p)\n", STR_LITERAL(lockptr), lockptr); \
  } \
  unlock(lockptr); \
})

/* barrier */
typedef struct {
  volatile uint64_t iteration;
  volatile uint64_t waiting;
  uint8_t current_state;
} bar_t;

#define EMPTY_BAR (bar_t){0,0,0}

void bwait(int cpu, int i, bar_t* barrier, int sz);

#define BWAIT(cpu, i, barrier, sz) ({ \
  if (DEBUG_BWAITS) { \
    debug("BWAIT(%s=%d, %s=%d, %s=%p, sz=%d)\n", STR_LITERAL(cpu), cpu, STR_LITERAL(i), i, STR_LITERAL(barrier), barrier, sz); \
  } \
  bwait(cpu, i, barrier, sz); \
})

#endif /* SYNC_H */
