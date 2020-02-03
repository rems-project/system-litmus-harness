/* mutex */
void lock(volatile int* lock);
void unlock(volatile int* lock);

/* barrier */
void bwait(int cpu, int i, volatile int* barrier, int sz);