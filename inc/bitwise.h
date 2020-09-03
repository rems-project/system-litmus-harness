#ifndef BITWISE_H
#define BITWISE_H

#include  <stdint.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

/* masks */
#define BITMASK(n) (((uint64_t)(1) << (n)) - 1)

/* bitwise operations */
#define BIT(x, i) (((x) >> (i)) & 0x1)
#define BIT_SLICE(x, top, bot) ((x >> (bot)) & ((1UL << (1UL + (top) - (bot))) - 1UL))
#define IS_ALIGNED(v, bits) (((v) & ((1UL << (bits)) - 1)) == 0)
#define IS_ALIGNED_TO(v, to) (((v) % (to)) == 0)
#define ALIGN_POW2(v, to) ((v) & ~((to) - 1))
#define ALIGN_TO(v, bits) ((v) & ~((1UL << (bits)) - 1))
#define ALIGN_UP(v, bits) (((v) + ((1UL << (bits)) - 1)) & ~((1UL << (bits)) - 1))
#define ALIGN_UP_TO(v, to) ((v) % (to) == 0 ? (v) : (v) + (((-v) % (to))) + (to))
#endif /* BITWISE_H */
