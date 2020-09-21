#ifndef LITMUS_IDXS_H
#define LITMUS_IDXS_H

#include <stdint.h>

/**
 * each run of a litmus test is an index into
 * various arrays of test data.
 */
typedef uint64_t run_idx_t;

/** unshuffled run index aka a counter up to now
 */
typedef uint64_t run_count_t;

/** output registers are stored as an array of 64-bit values
 * and that array can be indexed via a reg_idx_t
 */
typedef uint64_t reg_idx_t;

/** each variable is a 64-bit value
 * in an array, which can be indexed with a var_idx_t
 */
typedef uint64_t var_idx_t;

#endif /* LITMUS_IDXS_H */