#ifndef RAND_H
#define RAND_H
#include <stdint.h>

volatile uint64_t SEED;

void rand_seed(uint64_t seed);
uint64_t randn(void);
void shuffle(uint64_t* arr, int n);
#endif /* RAND_H */
