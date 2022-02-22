#ifndef RAND_H
#define RAND_H

extern volatile u64 INITIAL_SEED;
extern volatile u64 SEED;

void init_seed(void);
void reset_seed(void);
void rand_seed(u64 seed);
u64 randn(void);
u64 randrange(u64 low, u64 hi);
void shuffle(void* arr, u64 szof, u64 n);
#endif /* RAND_H */
