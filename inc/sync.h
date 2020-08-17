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

/* barrier */
typedef struct {
  volatile uint64_t counter;
  uint64_t released;
  uint8_t release_flags[4];
} bar_t;

void bwait(int cpu, int i, bar_t* barrier, int sz);
#endif /* SYNC_H */