#define num_of_threads 4

typedef struct {
  int entering[num_of_threads];
  unsigned int number[num_of_threads];
} lock_t;

/* mutex */
void lock(volatile lock_t* lock);
void unlock(volatile lock_t* lock);

/* barrier */
void bwait(int cpu, int i, volatile int* barrier, int sz);
