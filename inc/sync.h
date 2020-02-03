/* mutex */
void lock(volatile int* lock);
void unlock(volatile int* lock);

/* barrier */
void bwait(int cpu, int i, uint64_t* barrier, int sz);