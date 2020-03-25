#ifndef BITWISE_H
#define BITWISE_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

/* bitwise operations */
#define BIT(x, i) (((x) >> (i)) & 0x1)
#define BIT_SLICE(x, top, bot) ((x >> (bot)) & ((1UL << (1UL + (top) - (bot))) - 1UL))
#define IS_ALIGNED(v, bits) (((v) & ((1UL << (bits)) - 1)) == 0)
#define ALIGN_POW2(v, to) ((v) & ~((to) - 1))
#define ALIGN_TO(v, bits) ((v) & ~((1UL << (bits)) - 1))
#define ALIGN_UP(v, bits) (((v) + ((1UL << (bits)) - 1)) & ~((1UL << (bits)) - 1))
#endif /* BITWISE_H */
