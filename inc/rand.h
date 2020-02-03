#include <stdint.h>

volatile uint64_t SEED;

uint64_t read_clk(void);
void rand_seed(uint64_t seed);
uint64_t randn(void);
void shuffle(uint64_t* arr, int n);