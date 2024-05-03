#ifndef LITMUS_IDXS_H
#define LITMUS_IDXS_H

/**
 * each run of a litmus test is an index into
 * various arrays of test data.
 */
typedef u64 run_idx_t;

/** unshuffled run index aka a counter up to now
 */
typedef u64 run_count_t;

/** output registers are stored as an array of 64-bit values
 * and that array can be indexed via a reg_idx_t
 */
typedef u64 reg_idx_t;

/** each variable is a 64-bit value
 * in an array, which can be indexed with a var_idx_t
 */
typedef u64 var_idx_t;

#endif /* LITMUS_IDXS_H */