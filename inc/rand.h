#ifndef RAND_H
#define RAND_H
#include <stdint.h>

extern volatile uint64_t INITIAL_SEED;
extern volatile uint64_t SEED;

void reset_seed(void);
void rand_seed(uint64_t seed);
uint64_t randn(void);
void shuffle(void* arr, int szof, int n);
#endif /* RAND_H */
